#include "ws/frame.h"

#include <format>

namespace twilight::ws
{
std::string Frame::toString() const
{
  return std::format("Frame{{fin={}, rsv1={}, rsv2={}, rsv3={}, opcode=0x{:x}, payload=\"{}\"}}", fin, rsv1, rsv2, rsv3,
    static_cast<u8>(opcode), payload);
}
}  // namespace twilight::ws
