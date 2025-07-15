#pragma once

#include <functional>
#include <future>

#include "frame.h"
#include "http/client.h"
#include "uri.h"

namespace twilight::ws
{
class Client : protected http::Client
{
 private:
  template <typename... Args>
  class Signal
  {
   public:
    using Callback = std::function<void(Args...)>;

    inline void operator()(Args... args) noexcept
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (const auto& cb : callbacks) cb(std::forward<Args>(args)...);
    }

    inline void operator=(const Callback& cb) noexcept
    {
      std::lock_guard<std::mutex> lock(mutex);
      callbacks.push_back(std::move(cb));
    }

   protected:
    std::vector<Callback> callbacks;
    std::mutex mutex;
  };

 public:
  explicit Client(const URI& uri, http::ClientFlags flags = http::ClientFlags::None);

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  bool send(const char* str) const noexcept;
  bool send(const Frame& frame) const noexcept;

  Signal<const Frame&> onmessage;
  Signal<> onopen;
  Signal<> onclose;

  void connect();
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
