#include "http/response.h"

#include <brotli/decode.h>
#include <zlib.h>

#include <expected>
#include <vector>

#include "http/headers.h"

namespace twilight::http
{
std::expected<Response, std::string> Response::parse(const std::string &raw) noexcept
{
  usize headerEnd = raw.find("\r\n\r\n");
  if (headerEnd == std::string::npos)
    return std::unexpected("No CRLF separator found in response");

  std::string headerBlk = raw.substr(0, headerEnd);
  std::string body = raw.substr(headerEnd + 4);

  usize statusLineEnd = headerBlk.find("\r\n");
  if (statusLineEnd == std::string::npos)
    return std::unexpected("Invalid header line");

  std::string statusLine = headerBlk.substr(0, statusLineEnd);
  usize sp1 = statusLine.find(' ');
  usize sp2 = statusLine.find(' ', sp1 + 1);
  if (sp1 == std::string::npos || sp2 == std::string::npos)
    return std::unexpected("Invalid status line");

  u16 statusCode = std::stoul(statusLine.substr(sp1 + 1, sp2 - sp1 - 1));
  std::string statusMessage = statusLine.substr(sp2 + 1);

  auto headers = Headers::parse(headerBlk.substr(statusLineEnd + 2));
  if (!headers.has_value())
    return std::unexpected("Invalid header format");

  if (body.size()) {
    if (auto te = headers->get("transfer-encoding"); te.value_or("") == "chunked") {
      body = decodeChunked(body);
    }

    if (auto ce = headers->get("content-encoding"); ce.has_value()) {
      auto decompressed = decompress(ce.value(), body);
      if (!decompressed.has_value())
        return std::unexpected(decompressed.error());
      body = decompressed.value();
    }
  }

  return Response{.statusCode = statusCode, .statusMessage = statusMessage, .headers = headers.value(), .body = body};
}

std::string Response::decodeChunked(const std::string &data) noexcept
{
  std::string out;
  size_t pos = 0;
  while (pos < data.size()) {
    // Find the next CRLF
    size_t crlf = data.find("\r\n", pos);
    if (crlf == std::string::npos)
      break;
    // Parse chunk size
    std::string szstr = data.substr(pos, crlf - pos);
    size_t chunk_size = 0;
    try {
      chunk_size = std::stoul(szstr, nullptr, 16);
    } catch (...) {
      break;
    }
    if (chunk_size == 0)
      break;
    pos = crlf + 2;
    if (pos + chunk_size > data.size())
      break;
    out.append(data, pos, chunk_size);
    pos += chunk_size;
    // Skip CRLF after chunk
    if (data.substr(pos, 2) == "\r\n")
      pos += 2;
  }
  return out;
}

std::expected<std::string, std::string>
Response::decompress(const std::string &encoding, const std::string &data) noexcept
{
  if (encoding == "gzip" || encoding == "deflate") {
    // zlib/gzip/deflate
    z_stream zs{};
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = data.size();
    if (inflateInit2(&zs, encoding == "gzip" ? 16 + MAX_WBITS : MAX_WBITS) != Z_OK)
      return std::unexpected("Failed to initialize zlib stream");
    std::string out;
    char buf[4096];
    int ret;
    do {
      zs.next_out = reinterpret_cast<Bytef *>(buf);
      zs.avail_out = sizeof(buf);
      ret = inflate(&zs, Z_NO_FLUSH);
      if (ret != Z_OK && ret != Z_STREAM_END) {
        inflateEnd(&zs);
        return std::unexpected("Invalid compression");
      }
      out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret != Z_STREAM_END);
    inflateEnd(&zs);
    return out;
  } else if (encoding == "br") {
    // brotli
    BrotliDecoderState *s = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!s)
      return std::unexpected("Failed to initialize brotli stream");
    const uint8_t *next_in = reinterpret_cast<const uint8_t *>(data.data());
    size_t available_in = data.size();
    size_t out_len = data.size() * 3 + 1024;  // guess
    std::vector<uint8_t> out(out_len);
    uint8_t *next_out = out.data();
    size_t available_out = out.size();
    size_t total_out = 0;
    BrotliDecoderResult res;
    while (true) {
      res = BrotliDecoderDecompressStream(s, &available_in, &next_in, &available_out, &next_out, &total_out);
      if (res == BROTLI_DECODER_RESULT_ERROR) {
        BrotliDecoderDestroyInstance(s);
        return std::unexpected("Invalid compression");
      }
      if (res == BROTLI_DECODER_RESULT_SUCCESS)
        break;
      if (available_out == 0) {
        size_t used = next_out - out.data();
        out_len *= 2;
        out.resize(out_len);
        next_out = out.data() + used;
        available_out = out_len - used;
      }
    }
    size_t used = next_out - out.data();
    BrotliDecoderDestroyInstance(s);
    return std::string(reinterpret_cast<char *>(out.data()), used);
  } else {
    return std::unexpected("Unsupported encoding");
  }
}
}  // namespace twilight::http
