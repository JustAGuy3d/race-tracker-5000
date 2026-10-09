#pragma once
// PSRAM-backed helpers so large feed messages (tens of KB) stay out of internal RAM.

#include <ArduinoJson.h>
#include <esp_heap_caps.h>

#include <cstring>

struct SpiRamAllocator : ArduinoJson::Allocator {
  void* allocate(size_t n) override { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }
  void deallocate(void* p) override { heap_caps_free(p); }
  void* reallocate(void* p, size_t n) override {
    return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM);
  }
  static SpiRamAllocator* instance() {
    static SpiRamAllocator a;
    return &a;
  }
};

// Growable, NUL-terminated byte buffer for reassembling WebSocket fragments.
class MessageBuffer {
public:
  bool append(const uint8_t* data, size_t len) {
    if (len_ + len + 1 > cap_) {
      size_t newCap = cap_ ? cap_ : 64 * 1024;
      while (len_ + len + 1 > newCap) newCap *= 2;
      char* nb = static_cast<char*>(heap_caps_realloc(buf_, newCap, MALLOC_CAP_SPIRAM));
      if (!nb) {
        len_ = 0;
        return false;
      }
      buf_ = nb;
      cap_ = newCap;
    }
    memcpy(buf_ + len_, data, len);
    len_ += len;
    buf_[len_] = '\0';
    return true;
  }
  void clear() { len_ = 0; }
  char* data() { return buf_; }
  size_t size() const { return len_; }

private:
  char*  buf_ = nullptr;
  size_t len_ = 0;
  size_t cap_ = 0;
};
