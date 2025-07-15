#pragma once

#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>

#include "../uri.h"
#include "response.h"

namespace twilight::http
{
enum class Method : u8 {
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

enum class ClientFlags : int {
  None = 0,
  // don't connect to the server on construction
  NoConnect = 1 << 0,
  // don't follow redirects
  NoFollow = 1 << 1,
};

class Client
{
 public:
  explicit Client(const URI& uri, ClientFlags flags = ClientFlags::None);

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  Response request(const std::string& path, RequestInit opts = {});

  void connect();

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
  ClientFlags flags;
  std::atomic<bool> connected = false;

  isize recv(char* buf, usize len) const noexcept;
  isize send(const char* buf, usize len) const noexcept;

  bool sendAll(const std::string& msg) const noexcept;
  // !!! this method is designed for RFC 7230-compliant responses and won't work for anything else
  std::string recvAll() const noexcept;
};

Response fetch(const URI& uri, RequestInit opts = {});
}  // namespace twilight::http
