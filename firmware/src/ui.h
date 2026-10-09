#pragma once
#include <cstdint>
#include <ctime>

#include "display.h"
#include "f1state.h"
#include "schedule_parse.h"

constexpr int MENU_MAX_SERIES = 4;

// Next scheduled session, shown in the bar along the bottom of the leaderboard.
struct NextInfo {
  const char* series;  // nullptr: nothing scheduled / schedule not loaded
  uint32_t    accent;
  const char* text;    // e.g. "Qualifying  -  Sat 4:00 PM  (in 1h 12m)"
};

struct UiContext {
  bool        wifi;
  bool        live;           // active source connected and receiving
  const char* title;          // series name, e.g. "F1 ACADEMY"
  uint32_t    accent;         // series colour
  const char* driverHeading;  // "DRIVER" or "CAR"
  bool        autoMode;       // auto-switch on: show an AUTO tag
  const char* localTime;      // e.g. "2:35 PM"; empty until NTP has synced
  NextInfo    next;
};

void uiBegin(LGFX* lcd);
void uiMessage(const char* line1, const char* line2 = nullptr);
void uiRender(const F1State& st, const UiContext& ctx, time_t nowUtc);
void uiReset();  // clear the screen; the next uiRender() redraws everything

// ---- Schedule screens -----------------------------------------------------------
struct ScheduleView {
  const ScheduledSession* sessions;
  int         count;
  const char* seriesNames[MENU_MAX_SERIES];
  uint32_t    accents[MENU_MAX_SERIES];
  time_t      now;
  bool        idle;  // true: auto-switch "Up next" screen; false: menu page with CLOSE
  int         page;  // menu page: which page of the list (wraps around)
};

// Full-screen schedule. Redraws only when something visible changed.
void uiSchedule(const ScheduleView& v);
bool uiScheduleCloseHit(int x, int y);  // menu page: the CLOSE button

// ---- Menu -------------------------------------------------------------------
constexpr int MENU_BRIGHTNESS_LEVELS = 4;
extern const uint8_t kBrightnessLevels[MENU_BRIGHTNESS_LEVELS];

struct Logo;  // include/logos.h

struct MenuModel {
  const char* series[MENU_MAX_SERIES];
  const Logo* logos[MENU_MAX_SERIES];  // nullptr: show the name instead
  uint32_t    accents[MENU_MAX_SERIES];
  int         seriesCount;
  int         currentSeries;
  uint8_t     brightness;
  bool        autoMode;
  const char* wifiLine1;  // network name, or "Not connected"
  const char* wifiLine2;  // IP and signal
};

// series i -> i; brightness level j -> MENU_BRIGHTNESS + j
enum MenuHit {
  MENU_NONE = -1,
  MENU_CLOSE = 100,
  MENU_AUTO = 150,
  MENU_SCHEDULE = 160,
  MENU_WIFI = 170,
  MENU_BRIGHTNESS = 200,
};

void uiMenu(const MenuModel& m);
int uiMenuHit(const MenuModel& m, int x, int y);
