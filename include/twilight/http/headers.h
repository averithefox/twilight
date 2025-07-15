#pragma once

#include <expected>
#include <map>
#include <optional>
#include <string>

#include "utils/types.h"

namespace twilight::http
{
class Headers
{
 public:
  Headers() = default;
  Headers(const std::initializer_list<std::pair<std::string, std::string>>& headers) noexcept;
  Headers(const std::map<std::string, std::string>& headers) noexcept;

  void add(const std::string& key, const std::string& value) noexcept;
  void addIfNotExists(const std::string& key, const std::string& value) noexcept;
  std::optional<std::string> get(const std::string& key) const noexcept;

  // returns headers as a string in RFC 7230 compliant format (without the trailing CRLF)
  std::string toString() const noexcept;

  static std::expected<Headers, std::string> parse(const std::string& raw) noexcept;

 protected:
  std::map<std::string, std::string> headers;
};
}  // namespace twilight::http
