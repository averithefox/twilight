#pragma once

#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../uri.h"
#include "response.h"

namespace twilight::http
{
enum class Method {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  OPTIONS,
  HEAD,
};

struct RequestInit {
  Method method = Method::GET;
  std::string body{};
  Headers headers{};
};

class Client
{
 public:
  explicit Client(const URI& uri);

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  Response request(const std::string& path, RequestInit opts = {}) const;

 protected:
  struct Socket {
    int fd = -1;
    ~Socket();
  } sock;

  struct SSLCtx {
    SSL_CTX* ptr = nullptr;
    ~SSLCtx();
  } sslCtx;

  struct SSLHandle {
    SSL* ptr = nullptr;
    ~SSLHandle();
  } ssl;

  URI uri;

  isize recv(char* buf, usize len) const noexcept;
  isize send(const char* buf, usize len) const noexcept;

  bool sendAll(const std::string& msg) const noexcept;
  // !!! this method is designed for RFC 7230-compliant responses and won't work for anything else
  std::string recvAll() const noexcept;

  void connect();
};

Response fetch(const URI& uri, RequestInit opts = {});
}  // namespace twilight::http
