#pragma once
// Local-time formatting (TZ is set from config.h TIMEZONE in setup()).

#include <cstdio>
#include <ctime>

#include "config.h"

inline void fmtClock(time_t t, char* out, size_t n) {
  struct tm lt;
  localtime_r(&t, &lt);
#if CLOCK_24H
  snprintf(out, n, "%02d:%02d", lt.tm_hour, lt.tm_min);
#else
  snprintf(out, n, "%d:%02d %s", lt.tm_hour % 12 ? lt.tm_hour % 12 : 12, lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
#endif
}

// "Sat 4:30 PM"
inline void fmtDayClock(time_t t, char* out, size_t n) {
  struct tm lt;
  localtime_r(&t, &lt);
  char day[8], clock[12];
  strftime(day, sizeof day, "%a", &lt);
  fmtClock(t, clock, sizeof clock);
  snprintf(out, n, "%s %s", day, clock);
}

// "SATURDAY 24 OCT"
inline void fmtDayHeader(time_t t, char* out, size_t n) {
  struct tm lt;
  localtime_r(&t, &lt);
  strftime(out, n, "%A %d %b", &lt);
  for (char* p = out; *p; p++)
    if (*p >= 'a' && *p <= 'z') *p -= 32;
}

inline int localDay(time_t t) {
  struct tm lt;
  localtime_r(&t, &lt);
  return lt.tm_year * 400 + lt.tm_yday;
}

// "in 42m", "in 3h 12m", "in 2d 5h"
inline void fmtCountdown(long secs, char* out, size_t n) {
  if (secs < 0) secs = 0;
  const long m = (secs + 59) / 60;
  if (m < 60) snprintf(out, n, "in %ldm", m);
  else if (m < 24 * 60) snprintf(out, n, "in %ldh %02ldm", m / 60, m % 60);
  else snprintf(out, n, "in %ldd %ldh", m / 1440, (m % 1440) / 60);
}
