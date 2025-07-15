#pragma once

#include <expected>
#include <string>

#include "utils/types.h"
#include "headers.h"

namespace twilight::http
{
struct Response {
  u16 statusCode;
  std::string statusMessage;
  Headers headers;
  std::string body;

  inline bool ok() const noexcept { return statusCode >= 200 && statusCode < 300; }

  static std::expected<Response, std::string> parse(const std::string &raw) noexcept;

 private:
  static std::string decodeChunked(const std::string &data) noexcept;
  static std::expected<std::string, std::string>
  decompress(const std::string &encoding, const std::string &data) noexcept;
};
}  // namespace twilight::http
