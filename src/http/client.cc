#include "http/client.h"

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <format>
#include <memory>
#include <mutex>

#include "http/headers.h"

static constexpr const char* HTTP_VER = "HTTP/1.1";
static constexpr const char* USER_AGENT = "twilight/" VERSION;

namespace twilight::http
{
Client::Client(const URI& uri) : uri(uri)
{
  static std::once_flag sslInit;
  std::call_once(sslInit, [] { OPENSSL_init_ssl(0, nullptr); });
  connect();
}

void Client::connect()
{
  // DNS
  addrinfo hints{};
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  addrinfo* res = nullptr;
  int err = getaddrinfo(uri.host.c_str(), std::to_string(uri.port).c_str(), &hints, &res);
  if (err)
    throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(err)));

  auto resGuard = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>(res, &freeaddrinfo);

  // Connect
  for (addrinfo* p = res; p; p = p->ai_next) {
    sock.fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock.fd < 0)
      continue;
    if (::connect(sock.fd, p->ai_addr, p->ai_addrlen) == 0)
      break;
    ::close(sock.fd);
    sock.fd = -1;
  }
  if (sock.fd < 0)
    throw std::runtime_error("Unable to connect to " + uri.host);

  // TLS
  if (uri.isSecure()) {
    sslCtx.ptr = SSL_CTX_new(TLS_client_method());
    if (!sslCtx.ptr)
      throw std::runtime_error("SSL_CTX_new failed");
    SSL_CTX_set_verify(sslCtx.ptr, SSL_VERIFY_NONE, nullptr);

    ssl.ptr = SSL_new(sslCtx.ptr);
    SSL_set_fd(ssl.ptr, sock.fd);
    SSL_set_tlsext_host_name(ssl.ptr, uri.host.c_str());
    if (SSL_connect(ssl.ptr) != 1) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSL_connect failed");
    }
  }
}

isize Client::recv(char* buf, usize len) const noexcept
{
  return ssl.ptr ? SSL_read(ssl.ptr, buf, len) : ::recv(sock.fd, buf, len, 0);
}

isize Client::send(const char* buf, usize len) const noexcept
{
  return ssl.ptr ? SSL_write(ssl.ptr, buf, len) : ::send(sock.fd, buf, len, 0);
}

bool Client::sendAll(const std::string& msg) const noexcept
{
  usize off = 0, len = msg.size();
  while (off < len) {
    isize n = send(msg.data() + off, len - off);
    if (n <= 0) {
      if (ssl.ptr) {
        int e = SSL_get_error(ssl.ptr, n);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE)
          continue;
      }
      return false;
    }
    off += n;
  }
  return true;
}

std::string Client::recvAll() const noexcept
{
  constexpr isize CHUNK = 4096;
  std::string data;

  while (true) {
    char buf[CHUNK];
    isize n = recv(buf, CHUNK);
    if (n <= 0)
      break;
    data.append(buf, n);
    if (data.find("\r\n\r\n") != std::string::npos)
      break;
  }

  // If we never saw headers, return what we have
  usize hdrEnd = data.find("\r\n\r\n");
  if (hdrEnd == std::string::npos)
    return data;

  // Parse headers
  std::string headerBlk = data.substr(0, hdrEnd);
  auto hdrsExp = Headers::parse(headerBlk);
  if (!hdrsExp.has_value())
    return data;  // let Response::parse fail on malformed headers

  Headers& hdrs = *hdrsExp;
  usize bodySoFar = data.size() - (hdrEnd + 4);

  // Content-Length?
  if (auto cl = hdrs.get("Content-Length"); cl.has_value()) {
    usize want = std::stoul(*cl);
    while (bodySoFar < want) {
      char buf[CHUNK];
      isize n = recv(buf, CHUNK);
      if (n <= 0)
        break;
      data.append(buf, n);
      bodySoFar += n;
    }
  }  // chunked?
  else if (hdrs.get("Transfer-Encoding").value_or("") == "chunked") {
    // read until the zero‐length chunk terminator
    while (data.find("\r\n0\r\n\r\n", hdrEnd + 4) == std::string::npos) {
      char buf[CHUNK];
      isize n = recv(buf, CHUNK);
      if (n <= 0)
        break;
      data.append(buf, n);
    }
  }

  return data;
}

Response Client::request(const std::string& path, RequestInit opts) const
{
  std::string hostHdr = uri.host;
  if ((uri.port != 80 && uri.port != 443))
    hostHdr += ":" + std::to_string(uri.port);
  opts.headers.addIfNotExists("Host", hostHdr);
  opts.headers.addIfNotExists("User-Agent", USER_AGENT);
  opts.headers.addIfNotExists("Accept", "*/*");
  opts.headers.addIfNotExists("Accept-Encoding", "gzip, deflate, br");
  opts.headers.addIfNotExists("Connection", "keep-alive");
  if (!opts.body.empty())
    opts.headers.addIfNotExists("Content-Length", std::to_string(opts.body.size()));

  static const std::array<std::string, 7> M = {"GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS", "HEAD"};
  std::string mstr = M[usize(opts.method)];

  std::string msg = std::format("{} {} {}\r\n{}\r\n{}", mstr, path, HTTP_VER, opts.headers.toString(), opts.body);

  if (!sendAll(msg))
    throw std::runtime_error("Failed to send request");

  std::string raw = recvAll();
  auto res = Response::parse(raw);
  if (!res.has_value())
    throw std::runtime_error("Failed to parse response: " + res.error());
  return *res;
}

Client::Socket::~Socket()
{
  if (fd != -1) {
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
  }
}

Client::SSLCtx::~SSLCtx()
{
  if (ptr)
    SSL_CTX_free(ptr);
}

Client::SSLHandle::~SSLHandle()
{
  if (ptr)
    SSL_free(ptr);
}

Response fetch(const URI& uri, RequestInit opts)
{
  Client client{uri};
  return client.request(uri.path, std::move(opts));
}
}  // namespace twilight::http
