#pragma once

#include <functional>
#include <future>

#include "../http/client.h"
#include "../uri.h"
#include "frame.h"

namespace twilight::ws
{
enum class Event : u8 { Open, Close, Message };

template <Event E>
struct __WSListenerType {
  using type = std::conditional_t<E == Event::Message, void(const Frame&), void()>;
};

class Client : protected http::Client
{
 private:
  template <typename T>
  struct Callbacks {
    std::vector<T> callbacks;
    void operator=(const T& cb) noexcept { callbacks.push_back(std::move(cb)); }
  };

 public:
  explicit Client(const URI& uri);

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  bool send(const char* str) const noexcept;
  bool send(const Frame& frame) const noexcept;

  Callbacks<std::function<void(const Frame&)>> onmessage;
  Callbacks<std::function<void()>> onopen;
  Callbacks<std::function<void()>> onclose;

  void close() noexcept;

 protected:
  std::array<u8, 16> key;

  Frame recvFrame() const;

 private:
  std::atomic<bool> listening{false};
  std::future<void> listenFuture;

  void doHandshake();
  void listen() noexcept;
};
}  // namespace twilight::ws
