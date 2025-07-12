#pragma once

#include <string>

#include "types.h"

namespace twilight::base64
{
static constexpr const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline std::string encode(const std::string& input) noexcept
{
  std::string ret;

  usize sz = input.size();
  for (usize i = 0; i < sz; i += 3) {
    u8 char_array_3[3];
    u8 char_array_4[4];

    char_array_3[0] = input[i];
    char_array_3[1] = (i + 1 < sz) ? input[i + 1] : 0;
    char_array_3[2] = (i + 2 < sz) ? input[i + 2] : 0;

    char_array_4[0] = (char_array_3[0] & 0xFC) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) | ((char_array_3[1] & 0xF0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0F) << 2) | ((char_array_3[2] & 0xC0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3F;

    ret += chars[char_array_4[0]];
    ret += chars[char_array_4[1]];
    ret += chars[char_array_4[2]];
    ret += chars[char_array_4[3]];
  }

  if (sz % 3 == 1) {
    ret[ret.length() - 2] = '=';
    ret[ret.length() - 1] = '=';
  } else if (sz % 3 == 2) {
    ret[ret.length() - 1] = '=';
  }

  return ret;
}

template <usize N>
inline std::string encode(const std::array<u8, N>& input) noexcept
{
  std::string value(N, '\0');
  for (usize i = 0; i < N; ++i) {
    value[i] = input[i];
  }
  return encode(value);
}
}  // namespace twilight::base64
