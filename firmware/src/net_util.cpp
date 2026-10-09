#include "net_util.h"

#include <esp_heap_caps.h>
#include <rom/miniz.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <strings.h>

#include "psram.h"

bool httpsGet(const String& url, String& body, const char* headerName, const char* headerValue,
              uint16_t timeoutMs) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.setConnectTimeout(timeoutMs);
  if (!http.begin(client, url)) return false;
  if (headerName) http.addHeader(headerName, headerValue);
  const int code = http.GET();
  if (code == 200) body = http.getString();
  else Serial.printf("[net] GET %s -> %d\n", url.c_str(), code);
  http.end();
  return code == 200;
}

bool statusIsRunning(const char* status) {
  return status && (strcasecmp(status, "Started") == 0 || strcasecmp(status, "Aborted") == 0);
}

namespace {

// Stream adapter so HTTPClient::writeToStream() (which also undoes chunked
// transfer encoding) can fill a MessageBuffer.
class BufferStream : public Stream {
public:
  explicit BufferStream(MessageBuffer& b) : buf_(b) {}
  size_t write(uint8_t c) override { return buf_.append(&c, 1) ? 1 : 0; }
  size_t write(const uint8_t* data, size_t size) override { return buf_.append(data, size) ? size : 0; }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}

private:
  MessageBuffer& buf_;
};

}  // namespace

bool httpsGetBytes(const String& url, MessageBuffer& out, const char* headerName, const char* headerValue,
                   uint16_t timeoutMs) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.setConnectTimeout(timeoutMs);
  if (!http.begin(client, url)) return false;
  if (headerName) http.addHeader(headerName, headerValue);
  const int code = http.GET();
  bool ok = false;
  if (code == 200) {
    out.clear();
    BufferStream stream(out);
    ok = http.writeToStream(&stream) > 0 || out.size() > 0;
  } else {
    Serial.printf("[net] GET %s -> %d\n", url.c_str(), code);
  }
  http.end();
  return ok;
}

bool gunzip(const uint8_t* in, size_t len, MessageBuffer& out) {
  if (len < 18 || in[0] != 0x1f || in[1] != 0x8b || in[2] != 8) return false;
  const uint8_t flags = in[3];
  size_t pos = 10;
  if (flags & 4) pos += 2 + (in[pos] | (in[pos + 1] << 8));  // FEXTRA
  if (flags & 8) while (pos < len && in[pos++]) {}          // FNAME
  if (flags & 16) while (pos < len && in[pos++]) {}         // FCOMMENT
  if (flags & 2) pos += 2;                                  // FHCRC
  if (pos >= len - 8) return false;
  const uint32_t size = in[len - 4] | (in[len - 3] << 8) | (in[len - 2] << 16) | (uint32_t(in[len - 1]) << 24);
  if (size == 0 || size > 1024 * 1024) return false;

  auto* dst = static_cast<uint8_t*>(heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM));
  auto* decomp = static_cast<tinfl_decompressor*>(heap_caps_malloc(sizeof(tinfl_decompressor), MALLOC_CAP_SPIRAM));
  bool ok = false;
  if (dst && decomp) {
    tinfl_init(decomp);
    size_t inSize = len - 8 - pos;
    size_t outSize = size;
    const tinfl_status st = tinfl_decompress(decomp, in + pos, &inSize, dst, dst, &outSize,
                                             TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (st == TINFL_STATUS_DONE && outSize == size) {
      out.clear();
      ok = out.append(dst, size);
    }
  }
  heap_caps_free(decomp);
  heap_caps_free(dst);
  return ok;
}
