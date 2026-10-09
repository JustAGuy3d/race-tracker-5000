#include "pcc_parse.h"

#include <strings.h>

#include <cstdio>
#include <cstring>

#include "jsonutil.h"

using json::copyIf;
using json::copyOrClear;

namespace pcc {
namespace {

bool has(const char* s, const char* needle) {
  if (!s) return false;
  const size_t n = strlen(needle);
  for (; *s; s++)
    if (strncasecmp(s, needle, n) == 0) return true;
  return false;
}

// Class colours follow the Porsche live-timing site.
uint32_t classColour(const char* c) {
  if (!c || !*c) return 0x9A9AA6;
  if (has(c, "GTD") && has(c, "PRO")) return 0xE61D26;
  if (has(c, "GTD")) return 0x01AB51;
  if (has(c, "GTP")) return 0xC8C8C8;
  if (has(c, "LMP2")) return 0x5A82C0;
  if (has(c, "MASTER")) return 0xCD7F32;
  if (has(c, "ROOKIE")) return 0xF2A100;
  if (has(c, "PRO") && has(c, "AM")) return 0x3A6B9B;
  if (has(c, "PRO")) return 0xF2F2F2;
  if (has(c, "AM")) return 0x3A6B9B;
  static const uint32_t palette[] = {0x8E6CEF, 0x2EC4B6, 0xFF6B6B, 0xFFD166, 0x06D6A0};
  uint32_t h = 0;
  for (const char* p = c; *p; p++) h = h * 31 + static_cast<unsigned char>(*p);
  return palette[h % 5];
}

void normaliseType(const char* t, char (&out)[16]) {
  if (has(t, "RACE")) strcpy(out, "Race");
  else if (has(t, "QUAL")) strcpy(out, "Qualifying");
  else if (t && *t) strcpy(out, "Practice");
  else out[0] = '\0';
}

void trackFromMessage(const char* m, Session& s) {
  const char* code = "";
  if (has(m, "CHEQ") || has(m, "FINISH")) code = "C";
  else if (has(m, "RED")) code = "5";
  else if (has(m, "FCY") || has(m, "FULL COURSE")) code = "F";
  else if (has(m, "SAFETY")) code = "4";
  else if (has(m, "YELLOW")) code = "2";
  else if (has(m, "GREEN")) code = "1";
  strcpy(s.trackStatus, code);
  snprintf(s.trackMessage, sizeof s.trackMessage, "%s", m ? m : "");
}

void applyStatus(const char* status, Driver& d) {
  if (!status) return;
  if (has(status, "PIT OUT") || has(status, "OUT LAP")) d.pitOut = true;
  else if (has(status, "PIT") || has(status, "BOX")) d.inPit = true;
  else if (has(status, "RET") || has(status, "DNF") || has(status, "DNS") || has(status, "DSQ") ||
           has(status, "OUT"))
    d.retired = true;
  else if (has(status, "STOP")) d.stopped = true;
}

}  // namespace

void applyState(F1State& st, JsonObjectConst root) {
  Session& s = st.session;
  JsonObjectConst ses = root["session"];

  copyOrClear(root["meeting"]["title"], s.meeting);
  const char* series = ses["seriesName"];
  const char* name = ses["name"] | "";
  if (series && !has(series, "Carrera"))  // another series is on track at this event
    snprintf(s.name, sizeof s.name, "%s: %s", series, name);
  else
    snprintf(s.name, sizeof s.name, "%s", name);
  normaliseType(ses["type"], s.type);
  copyOrClear(ses["status"], s.status);
  copyOrClear(ses["timeRemaining"], s.clockRemaining);
  s.clockExtrapolating = false;  // polled every second; no local countdown needed
  s.currentLap = root["laps"]["current"] | 0;
  s.totalLaps = root["laps"]["total"] | 0;
  trackFromMessage(root["trackStatus"]["message"], s);

  // Every poll carries the full field, so rebuild it.
  st.clearDrivers();
  for (JsonObjectConst l : root["lines"].as<JsonArrayConst>()) {
    const char* num = l["racingNumber"];
    if (!num || !*num) continue;
    Driver* d = st.driver(num, true);
    if (!d) break;
    d->line = d->position = (l["lineNumber"] | 0) + 1;
    snprintf(d->tla, sizeof d->tla, "%s", num);
    d->teamColour = classColour(l["class"]);

    copyOrClear(l["gapToLeader"], d->gap);
    copyOrClear(l["intervalToPositionAhead"]["value"], d->interval);
    copyOrClear(l["lastLap"]["value"], d->lastLap);
    d->lastPersonalBest = l["lastLap"]["personalFastest"] | false;
    d->lastOverallBest = l["lastLap"]["overallFastest"] | false;
    copyOrClear(l["bestLap"]["value"], d->bestLap);
    d->numLaps = l["numberOfLaps"] | 0;
    d->pitStops = l["pitStops"] | 0;
    applyStatus(l["status"], *d);
  }

  // Practice/qualifying: show the feed's gap to the fastest car, or derive it from
  // best laps when the feed leaves it empty.
  if (!st.isRace()) {
    bool feedHasGaps = false;
    for (Driver& d : st.drivers) {
      if (!d.used || !d.gap[0]) continue;
      feedHasGaps = true;
      snprintf(d.diffFastest, sizeof d.diffFastest, "%s%s", d.gap[0] == '+' ? "" : "+", d.gap);
    }
    if (feedHasGaps) {
      st.touch();
      return;
    }

    long fastest = -1;
    for (const Driver& d : st.drivers) {
      long ms = d.used ? F1State::parseLapMs(d.bestLap) : -1;
      if (ms > 0 && (fastest < 0 || ms < fastest)) fastest = ms;
    }
    for (Driver& d : st.drivers) {
      long ms = d.used ? F1State::parseLapMs(d.bestLap) : -1;
      if (ms <= 0 || fastest <= 0) continue;
      if (ms == fastest) d.diffFastest[0] = '\0';
      else snprintf(d.diffFastest, sizeof d.diffFastest, "+%ld.%03ld", (ms - fastest) / 1000, (ms - fastest) % 1000);
    }
  }
  st.touch();
}

bool sessionRunning(const F1State& st) {
  return strcasecmp(st.session.status, "STARTED") == 0 || strcasecmp(st.session.status, "RUNNING") == 0;
}

bool carreraRunning(JsonObjectConst root) {
  const char* status = root["session"]["status"];
  const bool started = status && (strcasecmp(status, "STARTED") == 0 || strcasecmp(status, "RUNNING") == 0);
  return started && has(root["session"]["seriesName"], "Carrera") && !has(root["trackStatus"]["message"], "CHEQ");
}

}  // namespace pcc
