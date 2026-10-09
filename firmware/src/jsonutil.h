#pragma once
// Small helpers for applying partial JSON updates onto fixed C structs.
// Free of Arduino headers so the parsers can be unit-tested on a PC.

#include <ArduinoJson.h>
#include <cstdlib>
#include <cstring>

namespace json {

// Copies a string field only if it is present in the update (null leaves it alone).
inline void copyIf(JsonVariantConst v, char* dst, size_t n) {
  if (v.isNull()) return;
  const char* s = v.as<const char*>();
  if (!s) s = "";
  strncpy(dst, s, n - 1);
  dst[n - 1] = '\0';
}

template <size_t N>
void copyIf(JsonVariantConst v, char (&dst)[N]) { copyIf(v, dst, N); }

// Like copyIf, but a JSON null clears the field (for feeds that send full state).
template <size_t N>
void copyOrClear(JsonVariantConst v, char (&dst)[N]) {
  if (v.isNull()) dst[0] = '\0';
  else copyIf(v, dst, N);
}

// Accepts true/false, 0/1 and "true"/"false".
inline void boolIf(JsonVariantConst v, bool& dst) {
  if (v.is<bool>()) dst = v.as<bool>();
  else if (v.is<int>()) dst = v.as<int>() != 0;
  else if (v.is<const char*>()) dst = strcmp(v.as<const char*>(), "true") == 0 || strcmp(v.as<const char*>(), "1") == 0;
}

inline void intIf(JsonVariantConst v, int& dst) {
  if (v.is<int>()) dst = v.as<int>();
  else if (v.is<const char*>()) dst = atoi(v.as<const char*>());
}

// Feeds send arrays in full snapshots but {"<index>": {...}} objects in
// partial updates. This walks both forms.
template <typename F>
void forEachIndexed(JsonVariantConst v, F fn) {
  if (v.is<JsonArrayConst>()) {
    int i = 0;
    for (JsonVariantConst e : v.as<JsonArrayConst>()) fn(i++, e);
  } else if (v.is<JsonObjectConst>()) {
    for (JsonPairConst kv : v.as<JsonObjectConst>()) fn(atoi(kv.key().c_str()), kv.value());
  }
}

}  // namespace json
