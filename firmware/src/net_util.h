#pragma once
#include <Arduino.h>

// One-shot HTTPS GET (TLS without certificate checks; all feeds are public,
// read-only data). Returns true on HTTP 200 and fills `body`.
bool httpsGet(const String& url, String& body, const char* headerName = nullptr,
              const char* headerValue = nullptr, uint16_t timeoutMs = 8000);

// "Started" or "Aborted" (red flag) in the F1 / F2 / F3 / F1 Academy vocabulary.
bool statusIsRunning(const char* status);

class MessageBuffer;

// Like httpsGet, but keeps the raw bytes (e.g. a gzip body) in `out`.
bool httpsGetBytes(const String& url, MessageBuffer& out, const char* headerName = nullptr,
                   const char* headerValue = nullptr, uint16_t timeoutMs = 10000);

// Decompresses a gzip body (as api.formula1.com always sends) into `out`
// (NUL-terminated). Uses the decompressor in the ESP32-S3 ROM.
bool gunzip(const uint8_t* in, size_t len, MessageBuffer& out);
