#pragma once

#include <string>

#include "utils/types.h"

namespace twilight
{
struct URI {
  std::string protocol;
  std::string host;
  u16 port;
  std::string path;

  URI() = delete;
  URI(const char* url) noexcept;
  URI(std::string url) noexcept;

  bool isSecure() const noexcept;

  std::string toString() const noexcept;
};
}  // namespace twilight
