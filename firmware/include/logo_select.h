#pragma once
// The series logos are trademarks, so they are not in this repository.
// tools/make_logos.py downloads them and writes include/logos.h (ignored by git).
// Without that file the menu buttons show the series names instead.
#if __has_include("logos.h")
#include "logos.h"
#define HAVE_SERIES_LOGOS 1
#else
#include <cstdint>
struct Logo {
  uint16_t w, h;
  const uint16_t* rgb565;
};
#endif
