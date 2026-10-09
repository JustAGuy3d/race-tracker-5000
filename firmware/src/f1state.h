#pragma once
// Series-neutral timing model. F1, F1 Academy and Porsche Carrera Cup adapters
// all write into this; the UI only reads it.
// Deliberately free of Arduino headers so it can be unit-tested on a PC.

#include <ArduinoJson.h>
#include <cstdint>
#include <ctime>

constexpr int MAX_DRIVERS = 40;
constexpr int MAX_STINTS  = 10;

struct Driver {
  bool     used;
  char     num[4];
  char     tla[4];       // label shown in the driver column (TLA, or car number)
  uint32_t teamColour;   // 0xRRGGBB (team, or class for multi-class series)
  int      line;         // order from the feed
  int      position;     // 0 = unknown

  char gap[14];          // gap to leader (race)
  char interval[14];     // interval to car ahead (race)
  bool catching;

  char lastLap[12];
  bool lastPersonalBest;
  bool lastOverallBest;
  char bestLap[12];
  char diffFastest[14];  // practice/quali: gap to fastest

  char qBest[3][12];     // F1 qualifying: best lap per part
  char qDiff[3][14];     // F1 qualifying: gap to fastest per part

  bool inPit, pitOut, retired, stopped, knockedOut;
  int  numLaps, pitStops;

  char    compound[MAX_STINTS];   // 'S','M','H','I','W','?'
  int16_t stintLaps[MAX_STINTS];  // laps on that set
  bool    stintNew[MAX_STINTS];
  int     stintCount;
};

struct Session {
  int  key;
  char meeting[64];
  char name[48];      // "Race", "Qualifying", "Practice 1"...
  char type[16];      // normalised: "Race", "Qualifying", "Practice"
  char status[16];    // Inactive/Started/Finished/Finalised/...
  int  currentLap, totalLaps;
  int  sessionPart;   // F1 quali Q1/Q2/Q3
  char trackStatus[4];     // F1 codes: 1 green, 2 yellow, 4 SC, 5 red, 6 VSC, 7 VSC ending;
                           // also "C" chequered, "F" full-course yellow
  char trackMessage[24];
  char clockRemaining[12]; // "H:MM:SS"
  time_t clockUtc;
  bool clockExtrapolating; // counting down from clockUtc
};

class F1State {
public:
  Driver   drivers[MAX_DRIVERS];
  Session  session;
  uint32_t version = 0;  // incremented on every applied update

  F1State() { reset(); }
  void reset();
  void clearTiming();
  void clearDrivers();
  void touch() { version++; }

  // Applies one F1 live-timing topic payload (a full snapshot or a partial update).
  // Returns true if the session key changed (caller should re-subscribe).
  bool applyTopic(const char* topic, JsonVariantConst data);

  Driver* driver(const char* num, bool create);

  // Fills `out` with pointers to used drivers in running order; returns count.
  int sorted(const Driver** out) const;

  bool isRace() const;
  bool isQualifying() const;
  long clockSecondsLeft(time_t nowUtc) const;  // -1 if unknown

  static time_t parseIsoUtc(const char* s);
  static long parseLapMs(const char* s);  // "1:23.456" / "83.456" -> ms, -1 if not a time

private:
  void applyDriverList(JsonObjectConst o);
  void applyTimingData(JsonObjectConst o);
  void applyTimingAppData(JsonObjectConst o);
  void applySessionInfo(JsonObjectConst o, bool& keyChanged);
};
