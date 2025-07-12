#include "http/headers.h"

#include <algorithm>
#include <sstream>

#include "utils/types.h"

static std::string toLower(const std::string& s)
{
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  return lower;
}

static std::string trim(const std::string& s)
{
  auto b = s.find_first_not_of(" \t");
  if (b == std::string::npos)
    return "";
  auto e = s.find_last_not_of(" \t");
  return s.substr(b, e - b + 1);
}

namespace twilight::http
{
Headers::Headers(const std::initializer_list<std::pair<std::string, std::string>>& headers) noexcept
{
  for (const auto& [key, value] : headers) add(toLower(key), value);
}

Headers::Headers(const std::map<std::string, std::string>& headers) noexcept : headers(headers) {}

void Headers::add(const std::string& key, const std::string& value) noexcept { headers[toLower(key)] = value; }

void Headers::addIfNotExists(const std::string& key, const std::string& value) noexcept
{
  if (get(key).has_value())
    return;

  add(key, value);
}

std::optional<std::string> Headers::get(const std::string& key) const noexcept
{
  std::string lowerKey = toLower(key);
  if (headers.find(lowerKey) == headers.end())
    return std::nullopt;

  return headers.at(lowerKey);
}

std::string Headers::toString() const noexcept
{
  std::stringstream ss;
  for (const auto& [key, value] : headers) ss << key << ": " << value << "\r\n";
  return ss.str();
}

std::expected<Headers, std::string> Headers::parse(const std::string& raw) noexcept
{
  Headers hdrs;

  std::istringstream ss(raw);
  std::string line;

  while (std::getline(ss, line) && !line.empty()) {
    if (line.back() == '\r')
      line.pop_back();

    usize colonPos = line.find(':');
    if (colonPos == std::string::npos)
      continue;

    std::string name = trim(line.substr(0, colonPos));
    std::string value = trim(line.substr(colonPos + 1));

    hdrs.add(name, value);
  }

  return hdrs;
}
}  // namespace twilight::http
