#pragma once
// Session schedules for all series. Pure parsing (no Arduino headers) so it can
// be unit-tested on a PC; fetching lives in schedule.cpp.

#include <ArduinoJson.h>

#include <ctime>

struct ScheduledSession {
  int8_t series;   // index into the sources list (0 F1, 1 F1 Academy, 2 Porsche)
  bool   confirmed;
  char   name[28];
  time_t start;    // UTC
  time_t end;      // UTC
};

constexpr int MAX_SCHEDULE = 48;

namespace sched {

// "2026-10-22T12:25:00-05:00", "2026-10-23T17:30:00Z" or a date + "17:30:00Z".
time_t parseIso(const char* s);

// Adds sessions overlapping [from, to) to out[*n...]; returns entries added.
// Jolpica (Ergast-compatible) season: {"MRData":{"RaceTable":{"Races":[...]}}}
int addF1(JsonObjectConst root, int series, time_t from, time_t to, ScheduledSession* out, int* n, int max);
// api.formula1.com f2f3-fom-results/races: {"Races":[{"Sessions":[...]}]}
int addFa(JsonObjectConst root, int series, time_t from, time_t to, ScheduledSession* out, int* n, int max);
// Hand-entered sessions: name, local ISO start with offset, length in minutes.
struct ManualSession {
  const char* name;
  const char* start;
  int minutes;
};
int addManual(const ManualSession* list, int series, time_t from, time_t to, ScheduledSession* out, int* n,
              int max);

void sortByStart(ScheduledSession* s, int n);

// First session starting after `now` (optionally excluding a series); -1 if none.
int nextStarting(const ScheduledSession* s, int n, time_t now, int excludeSeries = -1);

}  // namespace sched
