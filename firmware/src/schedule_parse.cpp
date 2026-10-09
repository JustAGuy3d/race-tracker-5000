#include "schedule_parse.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "f1state.h"

namespace sched {
namespace {

bool add(ScheduledSession* out, int* n, int max, int series, const char* name, time_t start, time_t end,
         bool confirmed, time_t from, time_t to) {
  if (start <= 0 || end <= start || end < from || start >= to || *n >= max) return false;
  ScheduledSession& s = out[(*n)++];
  s.series = static_cast<int8_t>(series);
  s.confirmed = confirmed;
  snprintf(s.name, sizeof s.name, "%s", name);
  s.start = start;
  s.end = end;
  return true;
}

// Jolpica gives date and time separately: "2026-10-23" + "17:30:00Z".
time_t dateTime(JsonObjectConst o) {
  const char* d = o["date"];
  const char* t = o["time"];
  if (!d || !t) return 0;
  char buf[40];
  snprintf(buf, sizeof buf, "%sT%s", d, t);
  return parseIso(buf);
}

}  // namespace

time_t parseIso(const char* s) {
  const time_t base = F1State::parseIsoUtc(s);
  if (!base) return 0;
  // Optional UTC offset after the seconds (and any fraction): +hh:mm / -hh:mm / Z.
  const char* p = strchr(s, 'T');
  if (!p) return base;
  for (p++; *p && *p != '+' && *p != '-' && *p != 'Z'; p++) {}
  int sign = 0;
  if (*p == '+') sign = 1;
  else if (*p == '-') sign = -1;
  if (!sign) return base;
  int hh = 0, mm = 0;
  if (sscanf(p + 1, "%d:%d", &hh, &mm) < 1) return base;
  return base - sign * static_cast<time_t>(hh * 3600 + mm * 60);  // local -> UTC
}

int addF1(JsonObjectConst root, int series, time_t from, time_t to, ScheduledSession* out, int* n, int max) {
  static const struct {
    const char* key;
    const char* name;
    int minutes;
  } kSessions[] = {
      {"FirstPractice", "Practice 1", 60},       {"SecondPractice", "Practice 2", 60},
      {"ThirdPractice", "Practice 3", 60},       {"SprintQualifying", "Sprint Qualifying", 45},
      {"SprintShootout", "Sprint Shootout", 45}, {"Sprint", "Sprint", 60},
      {"Qualifying", "Qualifying", 60},
  };
  const int before = *n;
  for (JsonObjectConst race : root["MRData"]["RaceTable"]["Races"].as<JsonArrayConst>()) {
    for (const auto& k : kSessions) {
      JsonObjectConst o = race[k.key];
      if (o.isNull()) continue;
      const time_t start = dateTime(o);
      add(out, n, max, series, k.name, start, start + k.minutes * 60, true, from, to);
    }
    const time_t start = dateTime(race);  // the Grand Prix itself
    add(out, n, max, series, "Race", start, start + 120 * 60, true, from, to);
  }
  return *n - before;
}

int addFa(JsonObjectConst root, int series, time_t from, time_t to, ScheduledSession* out, int* n, int max) {
  const int before = *n;
  for (JsonObjectConst race : root["Races"].as<JsonArrayConst>()) {
    for (JsonObjectConst s : race["Sessions"].as<JsonArrayConst>()) {
      const time_t start = parseIso(s["SessionStartTime"] | "");
      time_t end = parseIso(s["SessionEndTime"] | "");
      if (end <= start) end = start + 45 * 60;
      add(out, n, max, series, s["SessionName"] | "Session", start, end, !(s["Unconfirmed"] | false), from, to);
    }
  }
  return *n - before;
}

int addManual(const ManualSession* list, int series, time_t from, time_t to, ScheduledSession* out, int* n,
              int max) {
  const int before = *n;
  for (const ManualSession* m = list; m && m->name; m++) {
    const time_t start = parseIso(m->start);
    add(out, n, max, series, m->name, start, start + m->minutes * 60, true, from, to);
  }
  return *n - before;
}

void sortByStart(ScheduledSession* s, int n) {
  std::sort(s, s + n, [](const ScheduledSession& a, const ScheduledSession& b) { return a.start < b.start; });
}

int nextStarting(const ScheduledSession* s, int n, time_t now, int excludeSeries) {
  for (int i = 0; i < n; i++)
    if (s[i].start > now && s[i].series != excludeSeries) return i;
  return -1;
}

}  // namespace sched
