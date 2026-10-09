#include "fa_parse.h"

#include <strings.h>

#include <cstring>

#include "jsonutil.h"

using json::boolIf;
using json::copyIf;
using json::intIf;

namespace fa {
namespace {

bool contains(const char* s, const char* needle) {
  if (!s) return false;
  const size_t n = strlen(needle);
  for (; *s; s++)
    if (strncasecmp(s, needle, n) == 0) return true;
  return false;
}

void applyLine(F1State& st, JsonObjectConst l) {
  const char* num = l["driver"]["RacingNumber"];
  if (!num || !*num) return;
  Driver* d = st.driver(num, true);
  if (!d) return;

  if (l["driver"]["TLA"].is<const char*>()) copyIf(l["driver"]["TLA"], d->tla);
  intIf(l["Number"], d->line);
  if (l["position"].is<JsonObjectConst>()) intIf(l["position"]["Value"], d->position);

  JsonObjectConst status = l["status"];
  if (!status.isNull()) {
    boolIf(status["Retired"], d->retired);
    boolIf(status["InPit"], d->inPit);
    boolIf(status["PitOut"], d->pitOut);
    boolIf(status["Stopped"], d->stopped);
  }

  if (l["gap"].is<JsonObjectConst>()) copyIf(l["gap"]["Value"], d->gap);
  if (l["interval"].is<JsonObjectConst>()) copyIf(l["interval"]["Value"], d->interval);
  if (l["gapP"].is<JsonObjectConst>()) copyIf(l["gapP"]["Value"], d->diffFastest);
  for (int q = 3; q >= 1; q--) {  // "F1-style" knockout qualifying
    char key[24];
    snprintf(key, sizeof key, "TimeDiffToFastestQ%d", q);
    if (l[key].is<JsonObjectConst>() && l[key]["Value"].as<const char*>() &&
        *l[key]["Value"].as<const char*>()) {
      copyIf(l[key]["Value"], d->diffFastest);
      break;
    }
  }

  if (l["laps"].is<JsonObjectConst>()) intIf(l["laps"]["Value"], d->numLaps);
  if (l["pits"].is<JsonObjectConst>()) intIf(l["pits"]["Value"], d->pitStops);

  JsonObjectConst last = l["last"];
  if (!last.isNull()) {
    copyIf(last["Value"], d->lastLap);
    boolIf(last["PersonalFastest"], d->lastPersonalBest);
    boolIf(last["OverallFastest"], d->lastOverallBest);
  }
  if (l["best"].is<JsonObjectConst>()) copyIf(l["best"]["Value"], d->bestLap);
}

void applyLines(F1State& st, JsonVariantConst lines) {
  if (lines.is<JsonObjectConst>()) {
    for (JsonPairConst kv : lines.as<JsonObjectConst>())
      if (kv.value().is<JsonObjectConst>()) applyLine(st, kv.value());
  } else if (lines.is<JsonArrayConst>()) {
    for (JsonVariantConst v : lines.as<JsonArrayConst>())
      if (v.is<JsonObjectConst>()) applyLine(st, v);
  }
}

void applyHeader(F1State& st, JsonObjectConst feed) {
  if (feed.isNull()) return;
  if (feed["UpdateType"] == "new") {  // new session: start from scratch
    st.clearDrivers();
    st.session.clockRemaining[0] = '\0';
    st.session.status[0] = '\0';
  }
  const char* session = feed["Session"];
  if (session) {
    Session& s = st.session;
    if (contains(session, "Race")) strcpy(s.type, "Race");
    else if (contains(session, "Qual")) strcpy(s.type, "Qualifying");
    else strcpy(s.type, "Practice");
    if (!s.name[0]) copyIf(feed["Session"], s.name);
  }
}

void applyData(F1State& st, JsonObjectConst feed, JsonVariantConst lines) {
  applyHeader(st, feed);
  applyLines(st, lines);
}

void applyStats(F1State& st, JsonVariantConst feed) {
  JsonVariantConst lines = feed["lines"].isNull() ? feed : feed["lines"];
  auto one = [&st](JsonVariantConst s) {
    const char* num = s["driver"]["RacingNumber"];
    if (!num) return;
    Driver* d = st.driver(num, true);
    if (d && s["PersonalBestLapTime"].is<JsonObjectConst>())
      copyIf(s["PersonalBestLapTime"]["Value"], d->bestLap);
  };
  if (lines.is<JsonObjectConst>()) {
    for (JsonPairConst kv : lines.as<JsonObjectConst>())
      if (kv.value().is<JsonObjectConst>()) one(kv.value());
  } else if (lines.is<JsonArrayConst>()) {
    for (JsonVariantConst v : lines.as<JsonArrayConst>()) one(v);
  }
}

void applyTime(F1State& st, JsonVariantConst sentTime, JsonVariantConst running, JsonVariantConst remaining) {
  Session& s = st.session;
  copyIf(remaining, s.clockRemaining);
  if (sentTime.is<const char*>()) s.clockUtc = F1State::parseIsoUtc(sentTime);
  boolIf(running, s.clockExtrapolating);
}

void applyRaceDetails(F1State& st, JsonObjectConst d) {
  if (d.isNull()) return;
  Session& s = st.session;
  const char* round = d["Round"] | "";
  const char* race = d["Race"] | "";
  snprintf(s.meeting, sizeof s.meeting, "%s%s%s", round, (*round && *race) ? " - " : "", race);
  copyIf(d["Session"], s.name);
}

}  // namespace

void applySnapshot(F1State& st, JsonObjectConst r) {
  if (r["racedetailsfeed"].is<JsonArrayConst>()) applyRaceDetails(st, r["racedetailsfeed"][1]);
  if (r["data"].is<JsonArrayConst>()) applyData(st, r["data"][1], r["data"][2]);
  if (r["statsfeed"].is<JsonArrayConst>()) applyStats(st, r["statsfeed"][1]);
  if (r["sessionfeed"].is<JsonArrayConst>()) copyIf(r["sessionfeed"][1]["Value"], st.session.status);
  if (r["trackfeed"].is<JsonArrayConst>()) {
    copyIf(r["trackfeed"][1]["Value"], st.session.trackStatus);
    copyIf(r["trackfeed"][1]["Message"], st.session.trackMessage);
  }
  if (r["timefeed"].is<JsonArrayConst>())
    applyTime(st, r["timefeed"][0], r["timefeed"][1], r["timefeed"][2]);
  st.touch();
}

void applyMessage(F1State& st, const char* method, JsonArrayConst a) {
  if (!method) return;
  if (!strcasecmp(method, "datafeed")) {
    JsonObjectConst feed = a[2];
    applyData(st, feed, feed["lines"]);
  } else if (!strcasecmp(method, "statsfeed")) {
    applyStats(st, a[1]);
  } else if (!strcasecmp(method, "sessionfeed")) {
    copyIf(a[1]["Value"], st.session.status);
  } else if (!strcasecmp(method, "trackfeed")) {
    copyIf(a[1]["Value"], st.session.trackStatus);
    copyIf(a[1]["Message"], st.session.trackMessage);
  } else if (!strcasecmp(method, "timefeed")) {
    applyTime(st, a[0], a[1], a[2]);
  } else if (!strcasecmp(method, "racedetailsfeed")) {
    applyRaceDetails(st, a[1]);
  } else {
    return;  // weather, commentary: not shown
  }
  st.touch();
}

}  // namespace fa
