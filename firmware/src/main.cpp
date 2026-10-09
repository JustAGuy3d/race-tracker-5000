#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include "auto_switch.h"
#include "config.h"
#include "display.h"
#include "f1feed.h"
#include "f1state.h"
#include "fa_feed.h"
#include "logo_select.h"
#include "pcc_feed.h"
#include "schedule.h"
#include "timefmt.h"
#include "touch_guard.h"
#include "ui.h"
#include "ui_theme.h"
#include "ui_wifi.h"
#include "wifi_store.h"

static LGFX lcd;
static F1State state;
static F1Feed f1Feed;
static FaFeed faFeed;
static PccFeed pccFeed;
static TimingSource* const sources[] = {&f1Feed, &faFeed, &pccFeed};
#ifdef HAVE_SERIES_LOGOS
static const Logo* const sourceLogos[] = {&LOGO_F1, &LOGO_FA, &LOGO_PCC};
#else
static const Logo* const sourceLogos[] = {nullptr, nullptr, nullptr};  // menu shows the names
#endif
static constexpr int SOURCE_COUNT = sizeof(sources) / sizeof(sources[0]);
static int current = 0;
static Preferences prefs;
static WiFiMulti* wifiMulti = nullptr;  // rebuilt when saved networks change
static uint8_t brightness = LCD_BRIGHTNESS;

// Screens. Dashboard is the leaderboard; UpNext is the schedule shown by
// auto-switch when nothing is on track; SchedulePage, WifiList and
// WifiKeyboard are opened from the menu and stay open until closed.
enum class Screen { Dashboard, Menu, SchedulePage, UpNext, WifiList, WifiKeyboard };
static Screen screen = Screen::Dashboard;
static int schedulePage = 0;
static constexpr uint32_t HOLD_MS = 900;

// Auto-switch
static bool autoMode = false;
static uint32_t lastSwitchMs = 0;
static constexpr uint32_t AUTO_MIN_DWELL_MS = 90000;  // stay put this long after any switch
static constexpr time_t PRE_SWITCH_S = 180;           // move to a series this long before its session
static constexpr time_t KEEP_RESULTS_S = 15 * 60;     // show results this long after a session ends

// Latest copy of the schedule (refreshed in the background).
static ScheduledSession sessions[MAX_SCHEDULE];
static int sessionCount = 0;

static TimingSource& active() { return *sources[current]; }

static void openWifiList(bool rescan);

static void showDashboard() {
  screen = Screen::Dashboard;
  uiReset();
}

static void activate(int index) {
  current = ((index % SOURCE_COUNT) + SOURCE_COUNT) % SOURCE_COUNT;
  lastSwitchMs = millis();
  autoProbeSetActive(current);
  prefs.putInt("series", current);
  state.reset();
  screen = Screen::Dashboard;
  uiMessage(active().title(), "Connecting to live timing...");
  Serial.printf("[main] series: %s\n", active().title());
  active().start(&state);
}

static void switchTo(int index) {
  if (index == current) return;
  active().stop();
  activate(index);
}

// ---- Schedule helpers -----------------------------------------------------------

// A session of `series` is on track, or starts within PRE_SWITCH_S.
static bool inScheduledWindow(time_t now, int series) {
  for (int i = 0; i < sessionCount; i++) {
    const ScheduledSession& s = sessions[i];
    if (s.series == series && now >= s.start - PRE_SWITCH_S && now < s.end) return true;
  }
  return false;
}

static bool endedRecently(time_t now, int series) {
  for (int i = 0; i < sessionCount; i++) {
    const ScheduledSession& s = sessions[i];
    if (s.series == series && now >= s.end && now < s.end + KEEP_RESULTS_S) return true;
  }
  return false;
}

static ScheduleView scheduleView(bool idle) {
  ScheduleView v = {};
  v.sessions = sessions;
  v.count = sessionCount;
  for (int i = 0; i < SOURCE_COUNT; i++) {
    v.seriesNames[i] = sources[i]->title();
    v.accents[i] = sources[i]->accent();
  }
  v.now = time(nullptr);
  v.idle = idle;
  v.page = schedulePage;
  return v;
}

static char nextText[80];

static NextInfo nextInfo(time_t now) {
  const int i = sched::nextStarting(sessions, sessionCount, now);
  if (i < 0 || now < 1700000000) return {nullptr, 0, ""};
  const ScheduledSession& s = sessions[i];
  char when[24], in[24];
  fmtDayClock(s.start, when, sizeof when);
  fmtCountdown(static_cast<long>(s.start - now), in, sizeof in);
  snprintf(nextText, sizeof nextText, "%s  -  %s  (%s)%s", s.name, when, in, s.confirmed ? "" : "  TBC");
  return {sources[s.series]->title(), sources[s.series]->accent(), nextText};
}

// ---- Auto-switch ------------------------------------------------------------------

static void setAutoMode(bool on) {
  autoMode = on;
  prefs.putBool("auto", on);
  autoProbeSetEnabled(on);
  Serial.printf("[auto] %s\n", on ? "on" : "off");
}

// Follows whatever is on track:
//  1. The series on screen is running (or its session is due): stay.
//  2. Another series is running (background check): switch to it.
//  3. Another series' session starts within 3 minutes (schedule): switch early.
//  4. Nothing on track or due: show the Up Next schedule (after results have
//     been up for 15 minutes).
static void autoSwitch() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 2000) return;
  lastCheck = millis();
  if (!autoMode) {
    if (screen == Screen::UpNext) showDashboard();
    return;
  }
  if (screen == Screen::Menu || screen == Screen::SchedulePage) return;

  const time_t now = time(nullptr);
  if (active().running(state) || inScheduledWindow(now, current)) {
    if (screen == Screen::UpNext) showDashboard();
    return;
  }
  if (millis() - lastSwitchMs < AUTO_MIN_DWELL_MS) return;

  for (int k = 1; k < SOURCE_COUNT; k++) {
    const int i = (current + k) % SOURCE_COUNT;
    if (autoProbeState(i) == 1) {
      Serial.printf("[auto] %s is on track, switching\n", sources[i]->title());
      switchTo(i);
      return;
    }
  }
  for (int k = 1; k < SOURCE_COUNT; k++) {
    const int i = (current + k) % SOURCE_COUNT;
    if (inScheduledWindow(now, i)) {
      Serial.printf("[auto] %s session due, switching\n", sources[i]->title());
      switchTo(i);
      return;
    }
  }
  if (screen == Screen::Dashboard && sessionCount > 0 && !endedRecently(now, current)) {
    Serial.println("[auto] nothing on track, showing Up Next");
    screen = Screen::UpNext;
  }
}

// ---- Menu ---------------------------------------------------------------------

static char wifiLine1[40];
static char wifiLine2[48];

static MenuModel menuModel() {
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(wifiLine1, sizeof wifiLine1, "%s", WiFi.SSID().c_str());
    snprintf(wifiLine2, sizeof wifiLine2, "%s   %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    snprintf(wifiLine1, sizeof wifiLine1, "Not connected");
    snprintf(wifiLine2, sizeof wifiLine2, "Trying known networks...");
  }

  MenuModel m = {};
  m.seriesCount = SOURCE_COUNT;
  for (int i = 0; i < SOURCE_COUNT; i++) {
    m.series[i] = sources[i]->menuName();
    m.logos[i] = sourceLogos[i];
    m.accents[i] = sources[i]->accent();
  }
  m.currentSeries = current;
  m.brightness = brightness;
  m.autoMode = autoMode;
  m.wifiLine1 = wifiLine1;
  m.wifiLine2 = wifiLine2;
  return m;
}

static void openMenu() {
  screen = Screen::Menu;
  uiMenu(menuModel());
}

static void openSchedulePage() {
  screen = Screen::SchedulePage;
  schedulePage = 0;
  uiReset();
  uiSchedule(scheduleView(false));
}

static void menuTap(int x, int y) {
  const MenuModel m = menuModel();
  const int hit = uiMenuHit(m, x, y);
  if (hit >= 0 && hit < SOURCE_COUNT) {
    if (autoMode) setAutoMode(false);  // picking a series by hand turns auto-switch off
    if (hit == current) showDashboard();
    else switchTo(hit);
  } else if (hit >= MENU_BRIGHTNESS && hit < MENU_BRIGHTNESS + MENU_BRIGHTNESS_LEVELS) {
    brightness = kBrightnessLevels[hit - MENU_BRIGHTNESS];
    lcd.setBrightness(brightness);
    prefs.putUChar("bright", brightness);
    uiMenu(menuModel());
  } else if (hit == MENU_AUTO) {
    setAutoMode(!autoMode);
    uiMenu(menuModel());
  } else if (hit == MENU_SCHEDULE) {
    openSchedulePage();
  } else if (hit == MENU_WIFI) {
    openWifiList(true);
  } else if (hit == MENU_CLOSE) {
    showDashboard();
  }
}

// ---- Wi-Fi setup screens ------------------------------------------------------------

static void rebuildWifiMulti() {
  delete wifiMulti;
  wifiMulti = new WiFiMulti();
  wifiMulti->addAP(WIFI_SSID, WIFI_PASSWORD);
  if (strlen(WIFI_SSID_2) > 0) wifiMulti->addAP(WIFI_SSID_2, WIFI_PASSWORD_2);
  wifiStoreAddTo(*wifiMulti);
}

static WifiNet nets[16];
static int netCount = 0;
static String savedNets[WIFI_SAVED_MAX];
static int savedCount = 0;
static char connectedSsid[33];

static WifiListView wifiListView(bool scanning) {
  savedCount = wifiStoreList(savedNets, WIFI_SAVED_MAX);
  snprintf(connectedSsid, sizeof connectedSsid, "%s", WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "");
  return {nets, netCount, savedNets, savedCount, connectedSsid, scanning};
}

static void openWifiList(bool rescan) {
  screen = Screen::WifiList;
  if (rescan) {
    uiWifiList(wifiListView(true));
    netCount = wifiScan(nets, 16);
    Serial.printf("[wifi] scan found %d networks\n", netCount);
  }
  uiWifiList(wifiListView(false));
}

// Keyboard state
static String kbText, kbSsid, kbTitle, kbStatus;
static int kbMode = KB_LETTERS;
static bool kbShift = false;
static bool kbAskingName = false;  // "Other": type the network name first
static uint32_t kbStatusColour = 0;

static KeyboardView keyboardView() {
  return {kbTitle.c_str(), kbText.c_str(), kbMode, kbShift, kbStatus.c_str(), kbStatusColour,
          kbAskingName ? "NEXT" : "CONNECT"};
}

static void openKeyboard(const String& ssid, bool askName) {
  screen = Screen::WifiKeyboard;
  kbSsid = ssid;
  kbAskingName = askName;
  kbText = "";
  kbMode = KB_LETTERS;
  kbShift = false;
  kbTitle = askName ? "Network name" : "Password for " + ssid;
  kbStatus = askName ? "Type the exact network name (upper/lower case matters)"
                     : "The password is shown as you type so you can check it";
  kbStatusColour = theme::C_DIM;
  uiKeyboard(keyboardView());
}

// Tries a network. On success it is saved and live timing restarts; on failure
// the previous connection is restored and the keyboard shows an error.
static bool connectTo(const String& ssid, const String& pass) {
  uiMessage("Connecting to", ssid.c_str());
  Serial.printf("[wifi] trying %s\n", ssid.c_str());
  active().stop();
  WiFi.disconnect();
  delay(200);
  WiFi.begin(ssid.c_str(), pass.c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(100);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] connected to %s, saved\n", ssid.c_str());
    wifiStoreRemember(ssid, pass);
    rebuildWifiMulti();
    activate(current);  // restart live timing, back to the leaderboard
    return true;
  }
  Serial.printf("[wifi] could not connect to %s\n", ssid.c_str());
  WiFi.disconnect();
  uiMessage("Couldn't connect", "Going back to the previous network...");
  wifiMulti->run(10000);
  active().start(&state);
  return false;
}

static void wifiListTap(int x, int y) {
  const int hit = uiWifiListHit(wifiListView(false), x, y);
  if (hit == WL_CLOSE) {
    showDashboard();
  } else if (hit == WL_RESCAN) {
    openWifiList(true);
  } else if (hit == WL_OTHER) {
    openKeyboard("", true);
  } else if (hit >= WL_FORGET && hit < WL_FORGET + WIFI_SAVED_MAX) {
    wifiStoreForget(hit - WL_FORGET);
    rebuildWifiMulti();
    openWifiList(false);
  } else if (hit >= 0 && hit < netCount) {
    if (nets[hit].secure) {
      openKeyboard(nets[hit].ssid, false);
    } else if (!connectTo(nets[hit].ssid, "")) {
      openWifiList(false);
    }
  }
}

static void keyboardTap(int x, int y) {
  const int key = uiKeyboardHit(keyboardView(), x, y);
  if (key == KEY_NONE) return;
  if (key == KEY_CANCEL) {
    openWifiList(false);
    return;
  }
  if (key > 0) {
    if (kbText.length() < 63) kbText += static_cast<char>(key);
    kbShift = false;  // one capital at a time, like a phone keyboard
  } else if (key == KEY_DEL) {
    if (kbText.length()) kbText.remove(kbText.length() - 1);
  } else if (key == KEY_SHIFT) {
    if (kbMode == KB_LETTERS) kbShift = !kbShift;
    else kbMode = kbMode == KB_SYMBOLS ? KB_SYMBOLS2 : KB_SYMBOLS;
  } else if (key == KEY_MODE) {
    kbMode = kbMode == KB_LETTERS ? KB_SYMBOLS : KB_LETTERS;
  } else if (key == KEY_OK) {
    if (kbAskingName) {
      if (!kbText.length()) return;
      const String name = kbText;
      openKeyboard(name, false);  // now the password (leave empty for an open network)
      return;
    }
    if (connectTo(kbSsid, kbText)) return;
    screen = Screen::WifiKeyboard;
    kbStatus = "Couldn't connect. Check the password (and that the network is 2.4 GHz).";
    kbStatusColour = theme::C_RED;
  }
  uiKeyboard(keyboardView());
}

// A tap (touch-down) at x, y on whichever screen is showing.
static void handleTap(int x, int y) {
  Serial.printf("[touch] %d,%d\n", x, y);
  switch (screen) {
    case Screen::Menu:
      menuTap(x, y);
      break;
    case Screen::SchedulePage:
      if (uiScheduleCloseHit(x, y)) {
        showDashboard();
      } else if (y > 54) {  // below the header: next page
        schedulePage++;
        uiSchedule(scheduleView(false));
      }
      break;
    case Screen::WifiList:
      wifiListTap(x, y);
      break;
    case Screen::WifiKeyboard:
      keyboardTap(x, y);
      break;
    default:
      break;  // leaderboard / Up Next: only press-and-hold does something
  }
}

// Press and hold anywhere for ~1 s to open the menu (a hold is hard to trigger
// by accident). Other screens respond to taps.
static void checkTouch() {
  static bool wasDown = false;
  static bool consumed = false;
  static uint32_t downSince = 0;
  int32_t x, y;
  const bool down = lcd.getTouch(&x, &y);

  if (down && !wasDown) {
    downSince = millis();
    consumed = false;
    if (screen != Screen::Dashboard && screen != Screen::UpNext) {
      handleTap(x, y);
      consumed = true;
    }
  }
  if (down && !consumed && millis() - downSince > HOLD_MS &&
      (screen == Screen::Dashboard || screen == Screen::UpNext)) {
    consumed = true;
    openMenu();
  }
  wasDown = down;
  // No timeouts: every screen stays open until it is closed by hand.
}

// Serial `p`: streams the screen as run-length-encoded RGB565 for tools/screenshot.py.
static void sendScreenshot() {
  const int w = lcd.width(), h = lcd.height();
  static uint16_t line[800];
  Serial.printf("\nSCREENSHOT %d %d\n", w, h);
  for (int y = 0; y < h; y++) {
    lcd.readRect(0, y, w, 1, reinterpret_cast<lgfx::rgb565_t*>(line));
    for (int x = 0; x < w;) {
      int run = 1;
      while (x + run < w && run < 255 && line[x + run] == line[x]) run++;
      const uint8_t rec[3] = {static_cast<uint8_t>(run), static_cast<uint8_t>(line[x] & 0xFF),
                              static_cast<uint8_t>(line[x] >> 8)};
      Serial.write(rec, 3);
      x += run;
    }
  }
  Serial.println("\nSCREENSHOT END");
}

// ---- Board / Wi-Fi ------------------------------------------------------------

static void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  rebuildWifiMulti();

  uiMessage("Connecting to Wi-Fi", "Hold the screen later for Menu > Wi-Fi to add a network");
  const uint32_t start = millis();
  while (wifiMulti->run(10000) != WL_CONNECTED && millis() - start < 40000) delay(500);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[wifi] not connected yet, will keep trying");
    // The ESP32-S3 is 2.4 GHz only; list what it can see to help diagnose.
    const int n = WiFi.scanNetworks();
    Serial.printf("[wifi] %d networks visible on 2.4 GHz:\n", n);
    for (int i = 0; i < n; i++)
      Serial.printf("[wifi]   %-32s ch %2d  %4d dBm\n", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
    WiFi.scanDelete();
  }
}

// Rejoin either known network if the connection drops (e.g. walking out of
// hotspot range and back).
static void keepWifi() {
  static uint32_t lastTry = 0;
  if (WiFi.status() == WL_CONNECTED || millis() - lastTry < 15000) return;
  lastTry = millis();
  Serial.println("[wifi] reconnecting...");
  if (wifiMulti->run(8000) == WL_CONNECTED)
    Serial.printf("[wifi] connected to %s\n", WiFi.SSID().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("[boot] PSRAM %u bytes free\n", (unsigned)ESP.getFreePsram());

  prefs.begin("f1cyd", false);
  brightness = prefs.getUChar("bright", LCD_BRIGHTNESS);

  wifiStoreBegin();
  touchWakeBoard();
  lcd.init();
  touchBegin(&lcd);
  lcd.setBrightness(brightness);
  uiBegin(&lcd);
  uiWifiBegin(&lcd);

  connectWifi();
  // Session clocks work in UTC (time() is always UTC); TZ only affects local
  // times shown on screen.
  configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");

  scheduleBegin();
  autoProbeBegin(sources, SOURCE_COUNT);
  setAutoMode(prefs.getBool("auto", DEFAULT_AUTO_SWITCH));

  activate(prefs.getInt("series", DEFAULT_SERIES));
}

void loop() {
  keepWifi();
  active().loop();
  touchWatch();
  checkTouch();
  autoSwitch();

  static uint32_t lastSnapshot = 0;
  if (millis() - lastSnapshot > 1000) {
    lastSnapshot = millis();
    sessionCount = scheduleSnapshot(sessions, MAX_SCHEDULE);
  }

  static uint32_t lastHeapLog = 0;
  if (millis() - lastHeapLog > 60000) {
    lastHeapLog = millis();
    Serial.printf("[heap] internal free %u (largest block %u), PSRAM free %u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), (unsigned)ESP.getFreePsram());
  }

  // Serial monitor: 0/1/2 pick a series, m menu, a auto-switch, s schedule page.
  if (Serial.available()) {
    const int c = Serial.read();
    if (c >= '0' && c < '0' + SOURCE_COUNT) {
      if (c - '0' == current) showDashboard();
      else switchTo(c - '0');
    } else if (c == 'm') {
      openMenu();
    } else if (c == 'a') {
      setAutoMode(!autoMode);
    } else if (c == 's') {
      openSchedulePage();
    } else if (c == 'n' && screen == Screen::SchedulePage) {  // same as tapping the list
      schedulePage++;
      uiSchedule(scheduleView(false));
    } else if (c == 'p') {
      sendScreenshot();
    } else if (c == 'w') {
      openWifiList(true);
    } else if (c == 'd') {
      touchReport();
    } else if (c == 't') {  // "t400,240": a tap at x, y (for testing screens over USB)
      int tx, ty;
      if (sscanf(Serial.readStringUntil('\n').c_str(), "%d,%d", &tx, &ty) == 2) handleTap(tx, ty);
    }
  }

  static uint32_t lastDraw = 0, lastVersion = 0;
  static bool lastFeed = false;
  const uint32_t now = millis();

  if (screen == Screen::UpNext || screen == Screen::SchedulePage) {
    if (now - lastDraw >= 1000) {
      lastDraw = now;
      uiSchedule(scheduleView(screen == Screen::UpNext));
    }
  } else if (screen == Screen::Dashboard) {
    const bool feedUp = active().live();
    // Redraw on new data (max ~6 fps) and once a second for the clocks.
    const bool changed = state.version != lastVersion || feedUp != lastFeed;
    if ((changed && now - lastDraw >= 150) || now - lastDraw >= 1000) {
      lastDraw = now;
      lastVersion = state.version;
      lastFeed = feedUp;
      const time_t utc = time(nullptr);
      char clock[16] = "";
      if (utc > 1700000000) fmtClock(utc, clock, sizeof clock);
      const UiContext ctx = {WiFi.status() == WL_CONNECTED, feedUp, active().title(), active().accent(),
                             active().driverHeading(), autoMode, clock, nextInfo(utc)};
      uiRender(state, ctx, utc);
    }
  }
  delay(1);
}
