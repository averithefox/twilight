#pragma once

#include <array>
#include <climits>
#include <string>

#include "types.h"

namespace twilight
{
static inline u32 rotl(u32 value, u32 count) noexcept
{
  const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
  count &= mask;
  return (value << count) | (value >> (-count & mask));
}

inline std::array<u8, 20> sha1(std::string data) noexcept
{
  usize ml = data.size() * 8;  // message length in bits
  data.push_back(0x80);

  while (data.size() % 64 != 56) {  // pad to 56 bytes (64 - sizeof(usize))
    data.push_back(0);
  }

  for (int i = 7; i >= 0; --i) {
    data.push_back(static_cast<u8>((ml >> (8 * i)) & 0xFF));
  }

  u32 h0 = 0x67452301;
  u32 h1 = 0xEFCDAB89;
  u32 h2 = 0x98BADCFE;
  u32 h3 = 0x10325476;
  u32 h4 = 0xC3D2E1F0;

  for (usize i = 0; i < data.size(); i += 64) {
    u32 w[80];
    for (usize j = 0; j < 16; ++j) {
      w[j] = (static_cast<u32>(static_cast<u8>(data[i + 4 * j])) << 24) |
             (static_cast<u32>(static_cast<u8>(data[i + 4 * j + 1])) << 16) |
             (static_cast<u32>(static_cast<u8>(data[i + 4 * j + 2])) << 8) |
             static_cast<u32>(static_cast<u8>(data[i + 4 * j + 3]));
    }

    for (usize j = 16; j < 80; ++j) {
      w[j] = rotl(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
    }

    u32 a = h0;
    u32 b = h1;
    u32 c = h2;
    u32 d = h3;
    u32 e = h4;
    u32 f, k;

    for (usize j = 0; j < 80; ++j) {
      if (j < 20) {
        f = (b & c) | (~b & d);
        k = 0x5A827999;
      } else if (j < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if (j < 60) {
        f = (b & c) ^ (b & d) ^ (c & d);
        k = 0x8F1BBCDC;
      } else if (j < 80) {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }

      u32 temp = rotl(a, 5) + f + e + k + w[j];
      e = d;
      d = c;
      c = rotl(b, 30);
      b = a;
      a = temp;
    }

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;
  }

  std::array<u8, 20> result;
  result[0] = (h0 >> 24) & 0xFF;
  result[1] = (h0 >> 16) & 0xFF;
  result[2] = (h0 >> 8) & 0xFF;
  result[3] = h0 & 0xFF;
  result[4] = (h1 >> 24) & 0xFF;
  result[5] = (h1 >> 16) & 0xFF;
  result[6] = (h1 >> 8) & 0xFF;
  result[7] = h1 & 0xFF;
  result[8] = (h2 >> 24) & 0xFF;
  result[9] = (h2 >> 16) & 0xFF;
  result[10] = (h2 >> 8) & 0xFF;
  result[11] = h2 & 0xFF;
  result[12] = (h3 >> 24) & 0xFF;
  result[13] = (h3 >> 16) & 0xFF;
  result[14] = (h3 >> 8) & 0xFF;
  result[15] = h3 & 0xFF;
  result[16] = (h4 >> 24) & 0xFF;
  result[17] = (h4 >> 16) & 0xFF;
  result[18] = (h4 >> 8) & 0xFF;
  result[19] = h4 & 0xFF;

  return result;
}
}  // namespace twilight
