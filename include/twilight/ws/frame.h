#pragma once

#include <string>

#include "../utils/types.h"

namespace twilight::ws
{
enum class Opcode : u8 {
  Continuation = 0x0,
  Text = 0x1,
  Binary = 0x2,
  Close = 0x8,
  Ping = 0x9,
  Pong = 0xA,
};

struct Frame {
  bool fin = true;
  bool rsv1 = false;
  bool rsv2 = false;
  bool rsv3 = false;
  Opcode opcode;
  std::string payload;

  std::string toString() const;
};
}  // namespace twilight::ws
