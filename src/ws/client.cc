#include "ws/client.h"

#include <endian.h>
#include <netdb.h>

#include "utils/base64.h"
#include "utils/bitwise.h"
#include "utils/random.h"
#include "utils/sha1.h"
#include "ws/frame.h"

namespace twilight::ws
{
Client::Client(const URI& uri, http::ClientFlags flags) : http::Client(uri, flags), key(rand<u8, 16>())
{
  if (!(flags & http::ClientFlags::NoConnect))
    connect();
}

bool Client::send(const char* str) const noexcept { return send({.opcode = Opcode::Text, .payload = str}); }

bool Client::send(const Frame& frame) const noexcept
{
  std::string data;

  // clang-format off
  u8 header = frame.fin << 7
    | frame.rsv1 << 6
    | frame.rsv2 << 5
    | frame.rsv3 << 4
    | (static_cast<u8>(frame.opcode) & 0b00001111);
  // clang-format on
  data.push_back(header);

  u8 len = 0b10000000;  // 1st bit: mask
  if (frame.payload.size() <= 125) {
    len |= static_cast<u8>(frame.payload.size());
  } else if (frame.payload.size() <= 0xFFFF) {
    len |= 126;
  } else {
    len |= 127;
  }
  data.push_back(len);

  if (frame.payload.size() > 125) {
    if (frame.payload.size() <= 0xFFFF) {
      u16 sz = htobe16(static_cast<u16>(frame.payload.size()));
      data.append(reinterpret_cast<char*>(&sz), sizeof(sz));
    } else {
      u64 sz = htobe64(frame.payload.size());
      data.append(reinterpret_cast<char*>(&sz), sizeof(sz));
    }
  }

  std::array<u8, 4> mask = rand<u8, 4>();
  data.append(reinterpret_cast<char*>(mask.data()), mask.size());

  for (usize i = 0; i < frame.payload.size(); ++i) data.push_back(frame.payload[i] ^ mask[i % 4]);

  return sendAll(data);
}

void Client::connect()
{
  doHandshake();
  onopen();
  listenFuture = std::async(std::launch::async, &Client::listen, this);
}

void Client::close() noexcept
{
  listening = false;
  send({.opcode = Opcode::Close, .payload = {}});
}

Frame Client::recvFrame() const
{
  constexpr isize CHUNK = 4096;
  std::string data;

  while (true) {
    char buf[CHUNK];
    isize n = recv(buf, CHUNK);
    if (n <= 0)
      break;
    data.append(buf, n);
    if (n < CHUNK)
      break;
  }

  Frame frame;

  if (data.size() < 2) {
    // not enough data
    return frame;
  }

  frame.fin = (data[0] & 0b10000000) >> 7;
  frame.rsv1 = (data[0] & 0b01000000) >> 6;
  frame.rsv2 = (data[0] & 0b00100000) >> 5;
  frame.rsv3 = (data[0] & 0b00010000) >> 4;
  frame.opcode = static_cast<Opcode>(data[0] & 0b00001111);

  usize len = data[1] & 0b01111111;

  usize lenSz = len == 126 ? 2 : len == 127 ? 8 : 0;
  if (data.size() < 2 + lenSz) {
    // not enough data
    return frame;
  }

  if (lenSz == 2) {
    len = be16toh(*reinterpret_cast<u16*>(data.data() + 2));
  } else if (lenSz == 8) {
    len = be64toh(*reinterpret_cast<u64*>(data.data() + 2));
  }

  if (data.size() < 2 + lenSz + len) {
    // not enough data
    return frame;
  }

  frame.payload.reserve(len);
  frame.payload.append(data.data() + 2 + lenSz, len);

  return frame;
}

void Client::doHandshake()
{
  std::string base64Key = base64::encode(key);
  http::Headers headers{
    {"Upgrade", "websocket"},
    {"Connection", "Upgrade"},
    {"Sec-WebSocket-Key", base64Key},
    {"Sec-WebSocket-Version", "13"},
  };

  http::Response res = request(uri.path, {.headers = headers});

  if (res.statusCode != 101)
    throw std::runtime_error("Failed to connect to WebSocket server. Status code: " + std::to_string(res.statusCode));

  std::string accept = base64::encode(sha1(base64Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
  if (res.headers.get("Sec-Websocket-Accept") != accept)
    throw std::runtime_error("Failed to connect to WebSocket server. Accept mismatch.");
}

void Client::listen() noexcept
{
  listening = true;

  while (listening) {
    Frame frame = recvFrame();

    switch (frame.opcode) {
    case Opcode::Close:
      onclose();
      break;
    case Opcode::Ping:
      send({.opcode = Opcode::Pong, .payload = frame.payload});
      break;
    case Opcode::Pong:
      break;
    default:
      onmessage(frame);
      break;
    }
  }
}
}  // namespace twilight::ws
