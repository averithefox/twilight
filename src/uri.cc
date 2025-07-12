#include "uri.h"

#include <cctype>
#include <format>
#include <string>

namespace twilight
{
URI::URI(const char* url) noexcept : URI(std::string(url)) {};

URI::URI(std::string url) noexcept
{
#define SET_STR(var, val) \
  if (var.empty())        \
    var = val;

  usize protocolPos = url.find("://");
  if (protocolPos != std::string::npos) {
    protocol = url.substr(0, protocolPos);
    url = url.substr(protocolPos + 3);
  } else {
    protocol = "http";
  }

  usize portPos = url.find(":");
  if (portPos != std::string::npos) {
    host = url.substr(0, portPos);
    std::string portStr;
    for (usize i = portPos + 1; i < url.size(); ++i) {
      if (std::isdigit(url[i])) {
        portStr += url[i];
      } else {
        break;
      }
    }
    port = std::stoi(portStr);
    url = url.substr(portPos + portStr.size() + 1);
  } else {
    port = isSecure() ? 443 : 80;
  }

  usize pathPos = url.find("/");
  if (pathPos != std::string::npos) {
    SET_STR(host, url.substr(0, pathPos));
    path = url.substr(pathPos);
  }

  if (!path.empty() && !host.empty())
    return;

  usize queryPos = url.find("?");
  if (queryPos != std::string::npos) {
    SET_STR(path, url.substr(queryPos));
    SET_STR(host, url.substr(0, queryPos));
  } else {
    SET_STR(path, "/");
    SET_STR(host, url);
  }
}

bool URI::isSecure() const noexcept { return protocol == "wss" || protocol == "https"; }

std::string URI::toString() const noexcept
{
  return std::format("URI{{protocol={}, host={}, port={}, path={}}}", protocol, host, port, path);
}
}  // namespace twilight
