#include "f1state.h"

#include <strings.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "jsonutil.h"

using json::boolIf;
using json::copyIf;
using json::forEachIndexed;
using json::intIf;

namespace {

char compoundLetter(const char* c) {
  if (!c || !*c) return '?';
  if (!strcmp(c, "SOFT")) return 'S';
  if (!strcmp(c, "MEDIUM")) return 'M';
  if (!strcmp(c, "HARD")) return 'H';
  if (!strcmp(c, "INTERMEDIATE")) return 'I';
  if (!strcmp(c, "WET")) return 'W';
  return '?';
}

long daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<long>(doe) - 719468;
}

}  // namespace

void F1State::reset() {
  memset(drivers, 0, sizeof(drivers));
  memset(&session, 0, sizeof(session));
  version++;
}

void F1State::clearDrivers() {
  memset(drivers, 0, sizeof(drivers));
  version++;
}

void F1State::clearTiming() {
  // Keep the driver list (numbers/TLAs/colours); wipe everything session-specific.
  for (Driver& d : drivers) {
    if (!d.used) continue;
    Driver keep = {};
    keep.used = true;
    memcpy(keep.num, d.num, sizeof keep.num);
    memcpy(keep.tla, d.tla, sizeof keep.tla);
    keep.teamColour = d.teamColour;
    keep.line = d.line;
    d = keep;
  }
  session.currentLap = session.totalLaps = session.sessionPart = 0;
  session.clockRemaining[0] = '\0';
  session.clockExtrapolating = false;
  version++;
}

Driver* F1State::driver(const char* num, bool create) {
  for (Driver& d : drivers)
    if (d.used && strcmp(d.num, num) == 0) return &d;
  if (!create) return nullptr;
  for (Driver& d : drivers) {
    if (d.used) continue;
    d = Driver{};
    d.used = true;
    strncpy(d.num, num, sizeof d.num - 1);
    return &d;
  }
  return nullptr;
}

bool F1State::applyTopic(const char* topic, JsonVariantConst data) {
  if (!topic || !data.is<JsonObjectConst>()) return false;
  JsonObjectConst o = data.as<JsonObjectConst>();
  bool keyChanged = false;

  if (!strcmp(topic, "DriverList")) {
    applyDriverList(o);
  } else if (!strcmp(topic, "TimingData")) {
    applyTimingData(o);
  } else if (!strcmp(topic, "TimingAppData")) {
    applyTimingAppData(o);
  } else if (!strcmp(topic, "SessionInfo")) {
    applySessionInfo(o, keyChanged);
  } else if (!strcmp(topic, "SessionStatus")) {
    copyIf(o["Status"], session.status);
  } else if (!strcmp(topic, "LapCount")) {
    intIf(o["CurrentLap"], session.currentLap);
    intIf(o["TotalLaps"], session.totalLaps);
  } else if (!strcmp(topic, "TrackStatus")) {
    copyIf(o["Status"], session.trackStatus);
    copyIf(o["Message"], session.trackMessage);
  } else if (!strcmp(topic, "ExtrapolatedClock")) {
    copyIf(o["Remaining"], session.clockRemaining);
    if (o["Utc"].is<const char*>()) session.clockUtc = parseIsoUtc(o["Utc"]);
    boolIf(o["Extrapolating"], session.clockExtrapolating);
  } else {
    return false;
  }
  version++;
  return keyChanged;
}

void F1State::applyDriverList(JsonObjectConst o) {
  for (JsonPairConst kv : o) {
    if (kv.key().c_str()[0] == '_' || !kv.value().is<JsonObjectConst>()) continue;
    Driver* d = driver(kv.key().c_str(), true);
    if (!d) continue;
    JsonObjectConst v = kv.value().as<JsonObjectConst>();
    copyIf(v["Tla"], d->tla);
    if (v["TeamColour"].is<const char*>())
      d->teamColour = strtoul(v["TeamColour"].as<const char*>(), nullptr, 16);
    intIf(v["Line"], d->line);
  }
}

void F1State::applyTimingData(JsonObjectConst o) {
  intIf(o["SessionPart"], session.sessionPart);

  JsonObjectConst lines = o["Lines"];
  if (lines.isNull()) return;
  for (JsonPairConst kv : lines) {
    if (!kv.value().is<JsonObjectConst>()) continue;
    Driver* d = driver(kv.key().c_str(), true);
    if (!d) continue;
    JsonObjectConst l = kv.value().as<JsonObjectConst>();

    intIf(l["Position"], d->position);
    intIf(l["Line"], d->line);
    copyIf(l["GapToLeader"], d->gap);

    JsonVariantConst iv = l["IntervalToPositionAhead"];
    if (iv.is<JsonObjectConst>()) {
      copyIf(iv["Value"], d->interval);
      boolIf(iv["Catching"], d->catching);
    } else {
      copyIf(iv, d->interval);
    }

    JsonVariantConst ll = l["LastLapTime"];
    if (ll.is<JsonObjectConst>()) {
      copyIf(ll["Value"], d->lastLap);
      boolIf(ll["PersonalFastest"], d->lastPersonalBest);
      boolIf(ll["OverallFastest"], d->lastOverallBest);
    }
    JsonVariantConst bl = l["BestLapTime"];
    if (bl.is<JsonObjectConst>()) copyIf(bl["Value"], d->bestLap);

    copyIf(l["TimeDiffToFastest"], d->diffFastest);

    forEachIndexed(l["BestLapTimes"], [d](int i, JsonVariantConst e) {
      if (i >= 0 && i < 3 && e.is<JsonObjectConst>()) copyIf(e["Value"], d->qBest[i]);
    });
    forEachIndexed(l["Stats"], [d](int i, JsonVariantConst e) {
      if (i >= 0 && i < 3 && e.is<JsonObjectConst>()) copyIf(e["TimeDiffToFastest"], d->qDiff[i]);
    });

    boolIf(l["InPit"], d->inPit);
    boolIf(l["PitOut"], d->pitOut);
    boolIf(l["Retired"], d->retired);
    boolIf(l["Stopped"], d->stopped);
    boolIf(l["KnockedOut"], d->knockedOut);
    intIf(l["NumberOfLaps"], d->numLaps);
    intIf(l["NumberOfPitStops"], d->pitStops);
  }
}

void F1State::applyTimingAppData(JsonObjectConst o) {
  JsonObjectConst lines = o["Lines"];
  if (lines.isNull()) return;
  for (JsonPairConst kv : lines) {
    if (!kv.value().is<JsonObjectConst>()) continue;
    Driver* d = driver(kv.key().c_str(), true);
    if (!d) continue;
    forEachIndexed(kv.value()["Stints"], [d](int i, JsonVariantConst s) {
      if (i < 0 || i >= MAX_STINTS || !s.is<JsonObjectConst>()) return;
      if (s["Compound"].is<const char*>()) d->compound[i] = compoundLetter(s["Compound"]);
      else if (!d->compound[i]) d->compound[i] = '?';
      if (s["TotalLaps"].is<int>()) d->stintLaps[i] = s["TotalLaps"].as<int>();
      boolIf(s["New"], d->stintNew[i]);
      if (i + 1 > d->stintCount) d->stintCount = i + 1;
    });
  }
}

void F1State::applySessionInfo(JsonObjectConst o, bool& keyChanged) {
  if (o["Key"].is<int>()) {
    int key = o["Key"].as<int>();
    if (session.key != 0 && key != session.key) {
      keyChanged = true;
      clearTiming();
    }
    session.key = key;
  }
  JsonObjectConst m = o["Meeting"];
  if (!m.isNull()) copyIf(m["Name"], session.meeting);
  copyIf(o["Name"], session.name);
  copyIf(o["Type"], session.type);
}

int F1State::sorted(const Driver** out) const {
  int n = 0;
  for (const Driver& d : drivers)
    if (d.used && d.line > 0) out[n++] = &d;
  std::sort(out, out + n, [](const Driver* a, const Driver* b) {
    int pa = a->position > 0 ? a->position : a->line;
    int pb = b->position > 0 ? b->position : b->line;
    if (pa != pb) return pa < pb;
    return a->line < b->line;
  });
  return n;
}

bool F1State::isRace() const { return strcasecmp(session.type, "Race") == 0; }

bool F1State::isQualifying() const { return strcasecmp(session.type, "Qualifying") == 0; }

long F1State::clockSecondsLeft(time_t nowUtc) const {
  int h, m, s;
  if (sscanf(session.clockRemaining, "%d:%d:%d", &h, &m, &s) != 3) return -1;
  long left = h * 3600L + m * 60L + s;
  if (session.clockExtrapolating && nowUtc > 1600000000 && session.clockUtc > 0)
    left -= static_cast<long>(nowUtc - session.clockUtc);
  return left < 0 ? 0 : left;
}

time_t F1State::parseIsoUtc(const char* s) {
  int Y, M, D, h, m, sec;
  if (!s || sscanf(s, "%d-%d-%dT%d:%d:%d", &Y, &M, &D, &h, &m, &sec) != 6) return 0;
  return static_cast<time_t>(daysFromCivil(Y, M, D) * 86400L + h * 3600L + m * 60L + sec);
}

long F1State::parseLapMs(const char* s) {
  if (!s || !*s) return -1;
  int m = 0;
  double sec = 0;
  if (strchr(s, ':')) {
    if (sscanf(s, "%d:%lf", &m, &sec) != 2) return -1;
  } else if (sscanf(s, "%lf", &sec) != 1) {
    return -1;
  }
  return static_cast<long>(m * 60000L + sec * 1000.0 + 0.5);
}
