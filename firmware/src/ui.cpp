#include "ui.h"

#include <algorithm>
#include <cstring>

#include "logo_select.h"
#include "timefmt.h"
#include "ui_theme.h"

// 800x480 layout: header, column labels, two columns of drivers, then a bar
// with the next scheduled session. Up to 22 cars use 11 rows of 34 px per
// column; bigger fields shrink the rows.
namespace {

constexpr int W = 800;
constexpr int H = 480;
constexpr int HEADER_H = 54;
constexpr int LABEL_H = 24;
constexpr int COL_W = 400;
constexpr int FOOTER_H = 26;  // "NEXT" bar
constexpr int ROWS_Y = HEADER_H + LABEL_H;
constexpr int ROWS_AREA = H - ROWS_Y - FOOTER_H - 2;
constexpr int MAX_ROWS_PER_COL = 20;
constexpr int MAX_SLOTS = MAX_ROWS_PER_COL * 2;

// Column anchors inside a 400 px half.
constexpr int X_POS_R = 32;   // right-aligned position
constexpr int X_BAR = 40;     // team/class colour bar
constexpr int X_TLA = 52;
constexpr int X_A_R = 172;    // INT (race) / BEST (other)
constexpr int X_B_R = 256;    // GAP
constexpr int X_C_R = 336;    // LAST
constexpr int X_TYRE = 356;   // tyre circle centre
constexpr int X_AGE = 374;

LGFX* lcd = nullptr;
LGFX_Sprite header;
LGFX_Sprite row;
LGFX_Sprite footer;
LGFX_Sprite screen;  // full-screen buffer for the schedule, so updates don't flicker

struct Layout {
  int  rowsPerCol;
  int  rowH;
  bool compact;
  bool operator!=(const Layout& o) const { return rowsPerCol != o.rowsPerCol; }
};

Layout layoutFor(int cars) {
  const int rpc = cars <= 22 ? 11 : std::min(MAX_ROWS_PER_COL, (cars + 1) / 2);
  const int rowH = std::min(36, ROWS_AREA / rpc);
  return {rpc, rowH, rowH < 30};
}

struct SlotCache {
  bool   valid;
  bool   hasDriver;
  bool   race;
  int    part;
  Driver d;
};
SlotCache cache[MAX_SLOTS];
Layout layout = {0, 0, false};
char lastLabels[48] = "";
char lastHeader[200] = "";
char lastFooter[160] = "";
uint32_t lastScheduleSig = 0;
bool emptyShown = false;

using namespace theme;

uint32_t compoundColour(char c) {
  switch (c) {
    case 'S': return 0xE8002D;
    case 'M': return 0xFFD12E;
    case 'H': return 0xF0F0F0;
    case 'I': return 0x43B02A;
    case 'W': return 0x0067AD;
    default:  return 0x777777;
  }
}

void invalidateRows() { memset(cache, 0, sizeof cache); }

void applyLayout(const Layout& l) {
  layout = l;
  row.deleteSprite();
  row.createSprite(COL_W, l.rowH);
  lcd->fillRect(0, ROWS_Y, W, H - ROWS_Y, rgb(C_BG));
  invalidateRows();
  lastFooter[0] = 0;
}

void drawLabels(bool race, bool tyres, const char* driverHeading) {
  char sig[sizeof lastLabels];
  snprintf(sig, sizeof sig, "%d|%d|%s", race, tyres, driverHeading);
  if (strcmp(sig, lastLabels) == 0) return;
  strcpy(lastLabels, sig);

  lcd->fillRect(0, HEADER_H, W, LABEL_H, rgb(C_BG));
  lcd->setFont(&fonts::FreeSans9pt7b);
  lcd->setTextColor(rgb(C_DIM));
  for (int col = 0; col < 2; col++) {
    const int x0 = col * COL_W;
    const int y = HEADER_H + LABEL_H / 2;
    lcd->setTextDatum(textdatum_t::middle_right);
    lcd->drawString("P", x0 + X_POS_R, y);
    lcd->drawString(race ? "INT" : "BEST", x0 + X_A_R, y);
    lcd->drawString("GAP", x0 + X_B_R, y);
    lcd->drawString("LAST", x0 + X_C_R, y);
    lcd->setTextDatum(textdatum_t::middle_left);
    lcd->drawString(driverHeading, x0 + X_TLA, y);
    if (tyres) lcd->drawString("TYRE", x0 + X_TYRE - 12, y);
  }
  invalidateRows();
}

struct Cell {
  const char* text;
  uint32_t colour;
};

Cell raceIntervalCell(const Driver& d) {
  if (d.retired) return {"OUT", C_DIM};
  if (d.stopped) return {"STOP", C_RED};
  if (d.inPit) return {"IN PIT", C_PIT};
  if (d.pitOut) return {"PIT OUT", C_PIT};
  return {d.interval, d.catching ? C_PB : C_TEXT};
}

void drawRow(int slot, const Driver* d, bool race, int part) {
  const int rowH = layout.rowH;
  const bool alt = (slot % layout.rowsPerCol) % 2;
  row.fillScreen(rgb(alt ? C_ROW_B : C_ROW_A));
  const int cy = rowH / 2;

  if (d) {
    const bool dim = d->retired || d->knockedOut;
    const uint32_t main = dim ? C_DIM : C_TEXT;
    const lgfx::IFont* bigFont = layout.compact ? &fonts::FreeSansBold9pt7b : &fonts::FreeSansBold12pt7b;

    char pos[4];
    snprintf(pos, sizeof pos, "%d", d->position > 0 ? d->position : d->line);
    row.setFont(bigFont);
    row.setTextColor(rgb(main));
    row.setTextDatum(textdatum_t::middle_right);
    row.drawString(pos, X_POS_R, cy);

    const int barInset = layout.compact ? 3 : 6;
    row.fillRect(X_BAR, barInset, 5, rowH - 2 * barInset, rgb(d->teamColour ? d->teamColour : 0x777777));

    row.setTextDatum(textdatum_t::middle_left);
    row.drawString(d->tla[0] ? d->tla : d->num, X_TLA, cy);

    Cell a, b;
    if (race) {
      a = raceIntervalCell(*d);
      b = {d->gap, main};
    } else {
      const int q = part >= 1 && part <= 3 ? part - 1 : -1;
      const char* best = (q >= 0 && d->qBest[q][0]) ? d->qBest[q] : d->bestLap;
      const char* diff = q >= 0 ? d->qDiff[q] : d->diffFastest;
      a = {best, main};
      b = {diff, main};
      if (d->inPit) b = {"IN PIT", C_PIT};
      else if (d->pitOut) b = {"PIT OUT", C_PIT};
    }
    const uint32_t lastColour = d->lastOverallBest ? C_PURPLE : d->lastPersonalBest ? C_PB : main;

    row.setFont(layout.compact ? &fonts::FreeSans9pt7b : &fonts::FreeSansBold9pt7b);
    row.setTextDatum(textdatum_t::middle_right);
    row.setTextColor(rgb(a.colour));
    row.drawString(a.text, X_A_R, cy);
    row.setTextColor(rgb(b.colour));
    row.drawString(b.text, X_B_R, cy);
    row.setTextColor(rgb(lastColour));
    row.drawString(d->lastLap, X_C_R, cy);

    if (d->stintCount > 0) {
      const int s = d->stintCount - 1;
      const uint32_t tc = rgb(compoundColour(d->compound[s]));
      const int r = std::min(12, rowH / 2 - 3);
      for (int k = 0; k < 3; k++) row.drawCircle(X_TYRE, cy, r - k, tc);
      char letter[2] = {d->compound[s], 0};
      row.setFont(&fonts::FreeSansBold9pt7b);
      row.setTextDatum(textdatum_t::middle_center);
      row.setTextColor(tc);
      row.drawString(letter, X_TYRE, cy + 1);

      char age[6];
      snprintf(age, sizeof age, "%d", d->stintLaps[s]);
      row.setFont(&fonts::FreeSans9pt7b);
      row.setTextDatum(textdatum_t::middle_left);
      row.setTextColor(rgb(C_DIM));
      row.drawString(age, X_AGE, cy);
    }
  }

  const int x = (slot / layout.rowsPerCol) * COL_W;
  const int y = ROWS_Y + (slot % layout.rowsPerCol) * rowH;
  row.pushSprite(lcd, x, y);
}

struct TrackLook {
  const char* text;
  uint32_t bg, fg;
};

TrackLook trackLook(const Session& s) {
  const char* t = s.trackStatus;
  if (!strcmp(t, "1")) return {"GREEN", 0x00A650, 0xFFFFFF};
  if (!strcmp(t, "2")) return {"YELLOW", 0xFFD400, 0x000000};
  if (!strcmp(t, "4")) return {"SAFETY CAR", 0xFF8C00, 0x000000};
  if (!strcmp(t, "5")) return {"RED FLAG", C_RED, 0xFFFFFF};
  if (!strcmp(t, "6")) return {"VSC", 0xFF8C00, 0x000000};
  if (!strcmp(t, "7")) return {"VSC ENDING", 0xFF8C00, 0x000000};
  if (!strcmp(t, "F")) return {"FCY", 0xFFD400, 0x000000};
  if (!strcmp(t, "C")) return {"CHEQUERED", 0xFFFFFF, 0x000000};
  return {s.trackMessage[0] ? s.trackMessage : "--", 0x3A3A44, 0xFFFFFF};
}

// Draws `text` at (x, y), shortened with "..." so it ends before `maxX`.
void drawFitted(LGFX_Sprite& g, const char* text, int x, int y, int maxX) {
  if (g.textWidth(text) <= maxX - x) {
    g.drawString(text, x, y);
    return;
  }
  char buf[96];
  snprintf(buf, sizeof buf, "%s", text);
  for (size_t n = strlen(buf); n > 0; n--) {
    buf[n] = 0;
    char tmp[100];
    snprintf(tmp, sizeof tmp, "%s...", buf);
    if (g.textWidth(tmp) <= maxX - x) {
      g.drawString(tmp, x, y);
      return;
    }
  }
}

void drawHeader(const F1State& st, const UiContext& ctx, time_t nowUtc) {
  const Session& s = st.session;
  const char* meeting = s.meeting[0] ? s.meeting : "Live Timing";

  char sub[80];
  snprintf(sub, sizeof sub, "%s%s%s", s.name, s.status[0] ? "  -  " : "", s.status);

  char big[24] = "";
  if (st.isRace() && s.totalLaps > 0) {
    snprintf(big, sizeof big, "LAP %d / %d", s.currentLap, s.totalLaps);
  } else {
    long left = st.clockSecondsLeft(nowUtc);
    if (left == 0 && !s.clockExtrapolating) left = -1;  // finished: no "0:00:00"
    if (left >= 0) snprintf(big, sizeof big, "%ld:%02ld:%02ld", left / 3600, (left / 60) % 60, left % 60);
  }
  if (st.isQualifying() && s.sessionPart > 0) {
    char withPart[32];
    snprintf(withPart, sizeof withPart, "Q%d  %s", s.sessionPart, big);
    strncpy(big, withPart, sizeof big - 1);
  }

  const TrackLook tl = trackLook(s);
  const uint32_t dot = ctx.live ? C_PB : ctx.wifi ? 0xFF8C00 : C_RED;

  char sig[sizeof lastHeader];
  snprintf(sig, sizeof sig, "%s|%d|%s|%s|%s|%s|%s|%06X", ctx.title, ctx.autoMode, meeting, sub, big, tl.text,
           ctx.localTime, (unsigned)dot);
  if (strcmp(sig, lastHeader) == 0) return;
  strcpy(lastHeader, sig);

  header.fillScreen(rgb(C_HEADER));
  header.fillRect(0, HEADER_H - 3, W, 3, rgb(ctx.accent));

  header.setTextDatum(textdatum_t::top_left);
  header.setFont(&fonts::FreeSansBold12pt7b);
  header.setTextColor(rgb(C_TEXT));
  const int textMaxX = big[0] ? 420 - 8 : 632;  // keep clear of the session clock and flag box
  drawFitted(header, meeting, 12, 5, textMaxX);

  header.setFont(&fonts::FreeSansBold9pt7b);
  header.setTextColor(rgb(ctx.accent));
  header.drawString(ctx.title, 12, 30);
  int subX = 12 + header.textWidth(ctx.title) + 10;
  if (ctx.autoMode) {
    const int tagW = header.textWidth("AUTO") + 10;
    header.fillRoundRect(subX - 2, 28, tagW, 18, 4, rgb(ctx.accent));
    header.setTextColor(rgb(C_HEADER));
    header.drawString("AUTO", subX + 3, 30);
    subX += tagW + 8;
  }
  header.setFont(&fonts::FreeSans9pt7b);
  header.setTextColor(rgb(C_DIM));
  drawFitted(header, sub, subX, 30, textMaxX);

  if (big[0]) {
    header.fillRect(420, 0, 212, HEADER_H - 3, rgb(C_HEADER));  // keep the clock clear of long names
    header.setFont(&fonts::FreeSansBold18pt7b);
    header.setTextColor(rgb(C_TEXT));
    header.setTextDatum(textdatum_t::middle_center);
    header.drawString(big, 526, HEADER_H / 2 - 1);
  }

  header.fillRoundRect(640, 4, 132, 28, 6, rgb(tl.bg));
  header.setFont(&fonts::FreeSansBold9pt7b);
  header.setTextColor(rgb(tl.fg));
  header.setTextDatum(textdatum_t::middle_center);
  header.drawString(tl.text, 706, 18);

  header.setTextColor(rgb(C_TEXT));
  header.drawString(ctx.localTime, 706, 42);  // local time of day under the flag

  header.fillCircle(787, 18, 6, rgb(dot));

  header.pushSprite(lcd, 0, 0);
}

void drawFooter(const NextInfo& next) {
  char sig[sizeof lastFooter];
  snprintf(sig, sizeof sig, "%s|%06X|%s", next.series ? next.series : "", (unsigned)next.accent,
           next.text ? next.text : "");
  if (strcmp(sig, lastFooter) == 0) return;
  strcpy(lastFooter, sig);

  footer.fillScreen(rgb(C_HEADER));
  if (next.series) {
    const int cy = FOOTER_H / 2;
    footer.setTextDatum(textdatum_t::middle_left);
    footer.setFont(&fonts::FreeSans9pt7b);
    footer.setTextColor(rgb(C_DIM));
    footer.drawString("NEXT", 12, cy);
    int x = 12 + footer.textWidth("NEXT") + 12;
    footer.fillRect(x, 6, 5, FOOTER_H - 12, rgb(next.accent));
    x += 12;
    footer.setFont(&fonts::FreeSansBold9pt7b);
    footer.setTextColor(rgb(next.accent));
    footer.drawString(next.series, x, cy);
    x += footer.textWidth(next.series) + 10;
    footer.setFont(&fonts::FreeSans9pt7b);
    footer.setTextColor(rgb(C_TEXT));
    footer.drawString(next.text, x, cy);
  }
  footer.pushSprite(lcd, 0, H - FOOTER_H);
}

}  // namespace

void uiBegin(LGFX* display) {
  lcd = display;
  header.setPsram(true);
  header.setColorDepth(16);
  header.createSprite(W, HEADER_H);
  row.setPsram(true);
  row.setColorDepth(16);
  footer.setPsram(true);
  footer.setColorDepth(16);
  footer.createSprite(W, FOOTER_H);
  screen.setPsram(true);
  screen.setColorDepth(16);
  lcd->fillScreen(rgb(C_BG));
}

void uiMessage(const char* line1, const char* line2) {
  lcd->fillScreen(rgb(C_BG));
  lcd->fillRect(0, 0, W, 6, rgb(C_RED));
  lcd->setTextDatum(textdatum_t::middle_center);
  lcd->setFont(&fonts::FreeSansBold18pt7b);
  lcd->setTextColor(rgb(C_TEXT));
  lcd->drawString(line1, W / 2, 220);
  if (line2) {
    lcd->setFont(&fonts::FreeSans12pt7b);
    lcd->setTextColor(rgb(C_DIM));
    lcd->drawString(line2, W / 2, 270);
  }
  invalidateRows();
  layout = {0, 0, false};
  lastLabels[0] = 0;
  lastHeader[0] = 0;
  lastFooter[0] = 0;
  lastScheduleSig = 0;
  emptyShown = false;
}

void uiRender(const F1State& st, const UiContext& ctx, time_t nowUtc) {
  drawHeader(st, ctx, nowUtc);
  lastScheduleSig = 0;  // the schedule screen must redraw fully when it comes back

  const Driver* order[MAX_DRIVERS];
  const int n = st.sorted(order);

  if (n == 0) {
    if (!emptyShown) {
      lcd->fillRect(0, HEADER_H, W, H - HEADER_H, rgb(C_BG));
      lcd->setTextDatum(textdatum_t::middle_center);
      lcd->setFont(&fonts::FreeSans12pt7b);
      lcd->setTextColor(rgb(C_DIM));
      lcd->drawString(ctx.live ? "Waiting for timing data..." : "Connecting to live timing...", W / 2, 260);
      lcd->setFont(&fonts::FreeSans9pt7b);
      lcd->drawString("Press and hold the screen to switch series", W / 2, 300);
      emptyShown = true;
      layout = {0, 0, false};
      lastLabels[0] = 0;
      lastFooter[0] = 0;
    }
    drawFooter(ctx.next);
    return;
  }
  if (emptyShown) {
    lcd->fillRect(0, HEADER_H, W, H - HEADER_H, rgb(C_BG));
    emptyShown = false;
    lastFooter[0] = 0;
  }

  const Layout want = layoutFor(n);
  if (want != layout) applyLayout(want);

  const bool race = st.isRace();
  bool tyres = false;
  for (int i = 0; i < n && !tyres; i++) tyres = order[i]->stintCount > 0;
  drawLabels(race, tyres, ctx.driverHeading);

  const int part = st.session.sessionPart;
  const int slots = layout.rowsPerCol * 2;
  for (int slot = 0; slot < slots; slot++) {
    const Driver* d = slot < n ? order[slot] : nullptr;
    SlotCache& c = cache[slot];
    const bool same = c.valid && c.race == race && c.part == part && c.hasDriver == (d != nullptr) &&
                      (!d || memcmp(&c.d, d, sizeof(Driver)) == 0);
    if (same) continue;
    drawRow(slot, d, race, part);
    c.valid = true;
    c.race = race;
    c.part = part;
    c.hasDriver = d != nullptr;
    if (d) memcpy(&c.d, d, sizeof(Driver));
  }
  drawFooter(ctx.next);
}

void uiReset() {
  uiMessage("");
  lcd->fillScreen(rgb(C_BG));
}

// ---- Menu -------------------------------------------------------------------

const uint8_t kBrightnessLevels[MENU_BRIGHTNESS_LEVELS] = {40, 100, 170, 255};

namespace {

constexpr int M_X = 20;
constexpr int M_W = W - 2 * M_X;
constexpr int M_GAP = 12;
constexpr int SERIES_Y = 92, SERIES_H = 140;
constexpr int BRIGHT_Y = 282, BRIGHT_H = 64;
constexpr int CLOSE_X = 600, CLOSE_Y = 374, CLOSE_W = W - M_X - 600, CLOSE_H = 84;

struct Rect {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

Rect seriesRect(int i, int n) {
  const int w = (M_W - (n - 1) * M_GAP) / n;
  return {M_X + i * (w + M_GAP), SERIES_Y, w, SERIES_H};
}

Rect brightRect(int j) {
  const int w = (M_W - (MENU_BRIGHTNESS_LEVELS - 1) * M_GAP) / MENU_BRIGHTNESS_LEVELS;
  return {M_X + j * (w + M_GAP), BRIGHT_Y, w, BRIGHT_H};
}

const Rect CLOSE_RECT = {CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H};
const Rect SCHEDULE_RECT = {CLOSE_X, 10, CLOSE_W, 50};
const Rect AUTO_RECT = {M_X, CLOSE_Y, 270, CLOSE_H};
constexpr int WIFI_X = M_X + 270 + 16;
const Rect WIFI_RECT = {WIFI_X - 4, CLOSE_Y, CLOSE_X - 12 - (WIFI_X - 4), CLOSE_H};

void sectionLabel(const char* text, int y, int x = M_X) {
  lcd->setFont(&fonts::FreeSans9pt7b);
  lcd->setTextColor(rgb(C_DIM));
  lcd->setTextDatum(textdatum_t::bottom_left);
  lcd->drawString(text, x, y - 8);
}

}  // namespace

void uiMenu(const MenuModel& m) {
  lastScheduleSig = 0;  // the schedule page draws over the menu next
  lcd->fillScreen(rgb(C_BG));

  lcd->setTextDatum(textdatum_t::middle_left);
  lcd->setFont(&fonts::FreeSansBold18pt7b);
  lcd->setTextColor(rgb(C_TEXT));
  lcd->drawString("MENU", M_X, 32);

  lcd->fillRoundRect(SCHEDULE_RECT.x, SCHEDULE_RECT.y, SCHEDULE_RECT.w, SCHEDULE_RECT.h, 10, rgb(0x3A3A48));
  lcd->setTextDatum(textdatum_t::middle_center);
  lcd->setFont(&fonts::FreeSansBold12pt7b);
  lcd->drawString("SCHEDULE", SCHEDULE_RECT.x + SCHEDULE_RECT.w / 2, SCHEDULE_RECT.y + SCHEDULE_RECT.h / 2);

  sectionLabel("SERIES", SERIES_Y);
  for (int i = 0; i < m.seriesCount; i++) {
    const Rect r = seriesRect(i, m.seriesCount);
    const bool on = i == m.currentSeries;
    // Logos are pre-blended onto C_ROW_B, so the button keeps that colour even
    // when selected; the selection shows as an accent border.
    lcd->fillRoundRect(r.x, r.y, r.w, r.h, 10, rgb(C_ROW_B));
    if (on)
      for (int k = 0; k < 3; k++) lcd->drawRoundRect(r.x + k, r.y + k, r.w - 2 * k, r.h - 2 * k, 10, rgb(m.accents[i]));

    const Logo* logo = m.logos[i];
    const int logoCy = r.y + (on ? r.h / 2 - 10 : r.h / 2);
    if (logo) {
      lcd->pushImage(r.x + (r.w - logo->w) / 2, logoCy - logo->h / 2, logo->w, logo->h,
                     reinterpret_cast<const lgfx::rgb565_t*>(logo->rgb565));
    } else {
      lcd->setTextDatum(textdatum_t::middle_center);
      lcd->setTextColor(rgb(C_TEXT));
      lcd->setFont(&fonts::FreeSansBold12pt7b);
      if (lcd->textWidth(m.series[i]) > r.w - 20) lcd->setFont(&fonts::FreeSansBold9pt7b);
      lcd->drawString(m.series[i], r.x + r.w / 2, logoCy);
    }
    if (on) {
      lcd->setTextDatum(textdatum_t::middle_center);
      lcd->setFont(&fonts::FreeSansBold9pt7b);
      lcd->setTextColor(rgb(m.accents[i]));
      lcd->drawString("SHOWING", r.x + r.w / 2, r.y + r.h - 22);
    }
  }

  sectionLabel("BRIGHTNESS", BRIGHT_Y);
  static const char* const names[MENU_BRIGHTNESS_LEVELS] = {"LOW", "MEDIUM", "HIGH", "MAX"};
  for (int j = 0; j < MENU_BRIGHTNESS_LEVELS; j++) {
    const Rect r = brightRect(j);
    const bool on = kBrightnessLevels[j] == m.brightness;
    lcd->fillRoundRect(r.x, r.y, r.w, r.h, 10, rgb(on ? 0x3A3A48 : C_ROW_B));
    if (on) lcd->drawRoundRect(r.x, r.y, r.w, r.h, 10, rgb(C_TEXT));
    lcd->setTextDatum(textdatum_t::middle_center);
    lcd->setFont(&fonts::FreeSansBold12pt7b);
    lcd->setTextColor(rgb(on ? C_TEXT : C_DIM));
    lcd->drawString(names[j], r.x + r.w / 2, r.y + r.h / 2);
  }

  sectionLabel("AUTO-SWITCH", CLOSE_Y);
  lcd->fillRoundRect(AUTO_RECT.x, AUTO_RECT.y, AUTO_RECT.w, AUTO_RECT.h, 10, rgb(m.autoMode ? 0x1E6B3C : C_ROW_B));
  if (m.autoMode) lcd->drawRoundRect(AUTO_RECT.x, AUTO_RECT.y, AUTO_RECT.w, AUTO_RECT.h, 10, rgb(C_PB));
  lcd->setTextDatum(textdatum_t::middle_center);
  lcd->setFont(&fonts::FreeSansBold12pt7b);
  lcd->setTextColor(rgb(m.autoMode ? C_TEXT : C_DIM));
  lcd->drawString(m.autoMode ? "ON" : "OFF", AUTO_RECT.x + AUTO_RECT.w / 2, AUTO_RECT.y + 30);
  lcd->setFont(&fonts::FreeSans9pt7b);
  lcd->drawString("Follows what is on track", AUTO_RECT.x + AUTO_RECT.w / 2, AUTO_RECT.y + 60);

  sectionLabel("WI-FI", CLOSE_Y, WIFI_X);
  lcd->fillRoundRect(WIFI_RECT.x, WIFI_RECT.y, WIFI_RECT.w, WIFI_RECT.h, 10, rgb(C_ROW_B));
  lcd->setTextDatum(textdatum_t::top_left);
  lcd->setFont(&fonts::FreeSansBold9pt7b);
  lcd->setTextColor(rgb(C_TEXT));
  lcd->drawString(m.wifiLine1, WIFI_X + 8, CLOSE_Y + 8);
  lcd->setFont(&fonts::FreeSans9pt7b);
  lcd->drawString(m.wifiLine2, WIFI_X + 8, CLOSE_Y + 32);
  lcd->setTextColor(rgb(C_PIT));
  lcd->drawString("Tap to change network", WIFI_X + 8, CLOSE_Y + 58);

  lcd->fillRoundRect(CLOSE_RECT.x, CLOSE_RECT.y, CLOSE_RECT.w, CLOSE_RECT.h, 10, rgb(0x3A3A48));
  lcd->setTextDatum(textdatum_t::middle_center);
  lcd->setFont(&fonts::FreeSansBold12pt7b);
  lcd->setTextColor(rgb(C_TEXT));
  lcd->drawString("CLOSE", CLOSE_RECT.x + CLOSE_RECT.w / 2, CLOSE_RECT.y + CLOSE_RECT.h / 2);
}

int uiMenuHit(const MenuModel& m, int x, int y) {
  for (int i = 0; i < m.seriesCount; i++)
    if (seriesRect(i, m.seriesCount).contains(x, y)) return i;
  for (int j = 0; j < MENU_BRIGHTNESS_LEVELS; j++)
    if (brightRect(j).contains(x, y)) return MENU_BRIGHTNESS + j;
  if (CLOSE_RECT.contains(x, y)) return MENU_CLOSE;
  if (AUTO_RECT.contains(x, y)) return MENU_AUTO;
  if (WIFI_RECT.contains(x, y)) return MENU_WIFI;
  if (SCHEDULE_RECT.contains(x, y)) return MENU_SCHEDULE;
  return MENU_NONE;
}

// ---- Schedule screen --------------------------------------------------------

namespace {

uint32_t fnv(uint32_t h, const char* s) {
  for (; *s; s++) h = (h ^ static_cast<uint8_t>(*s)) * 16777619u;
  return (h ^ '|') * 16777619u;
}

// Status text/colour for a session at `now`.
void sessionStatus(const ScheduledSession& s, time_t now, char* out, size_t n, uint32_t* colour) {
  if (now >= s.end) {
    snprintf(out, n, "DONE");
    *colour = C_DIM;
  } else if (now >= s.start) {
    snprintf(out, n, "ON TRACK");
    *colour = C_PB;
  } else {
    fmtCountdown(static_cast<long>(s.start - now), out, n);
    if (!s.confirmed) strncat(out, "  TBC", n - strlen(out) - 1);
    *colour = C_TEXT;
  }
}

constexpr int SCHED_HERO_Y = HEADER_H + 10, SCHED_HERO_H = 88;
constexpr int SCHED_LIST_Y = SCHED_HERO_Y + SCHED_HERO_H + 10;
constexpr int SCHED_ROW = 26, SCHED_DAY = 24;

// Index just past the last session that fits on a page starting at `start`
// (each page starts with its own day heading).
int pageEnd(const ScheduleView& v, int start) {
  int y = SCHED_LIST_Y, lastDay = -1, i = start;
  for (; i < v.count; i++) {
    const int day = localDay(v.sessions[i].start);
    if (day != lastDay) {
      if (y + SCHED_DAY + SCHED_ROW > H - 4) break;
      y += SCHED_DAY;
      lastDay = day;
    }
    if (y + SCHED_ROW > H - 4) break;
    y += SCHED_ROW;
  }
  return i > start ? i : start + 1;  // always make progress
}

const Rect SCHED_CLOSE = {640, 8, 140, 38};

}  // namespace

bool uiScheduleCloseHit(int x, int y) { return SCHED_CLOSE.contains(x, y); }

void uiSchedule(const ScheduleView& v) {
  const time_t now = v.now;

  // Hero: the session on track now, else the next one to start.
  int hero = -1;
  for (int i = 0; i < v.count && hero < 0; i++)
    if (v.sessions[i].start <= now && now < v.sessions[i].end) hero = i;
  if (hero < 0) hero = sched::nextStarting(v.sessions, v.count, now);

  // List: from the first session that ended less than 3 hours ago.
  int first = 0;
  while (first < v.count && v.sessions[first].end < now - 3 * 3600) first++;

  // Pages of the list (the Up Next screen always shows the first).
  int pageStarts[16], pages = 0;
  for (int i = first; i < v.count && pages < 16; i = pageEnd(v, i)) pageStarts[pages++] = i;
  const int page = pages ? (v.idle ? 0 : v.page % pages) : 0;
  const int listStart = pages ? pageStarts[page] : first;
  const int listEnd = pages ? pageEnd(v, listStart) : first;

  // Skip the redraw if nothing visible changed (countdowns move once a minute).
  char buf[64];
  uint32_t sig = 2166136261u;
  sig = fnv(sig, v.idle ? "idle" : "page");
  snprintf(buf, sizeof buf, "%d/%d", page, pages);
  sig = fnv(sig, buf);
  fmtDayClock(now, buf, sizeof buf);
  sig = fnv(sig, buf);
  for (int i = first; i < v.count; i++) {
    uint32_t c;
    sessionStatus(v.sessions[i], now, buf, sizeof buf, &c);
    sig = fnv(fnv(sig, buf), v.sessions[i].name);
  }
  if (hero >= 0) sig = fnv(sig, v.sessions[hero].name);
  if (sig == lastScheduleSig) return;
  lastScheduleSig = sig;

  if (!screen.getBuffer()) screen.createSprite(W, H);
  LGFX_Sprite& g = screen;
  g.fillScreen(rgb(C_BG));

  // Header
  g.fillRect(0, 0, W, HEADER_H, rgb(C_HEADER));
  g.setTextDatum(textdatum_t::middle_left);
  g.setFont(&fonts::FreeSansBold18pt7b);
  g.setTextColor(rgb(C_TEXT));
  g.drawString(v.idle ? "UP NEXT" : "SCHEDULE", 20, HEADER_H / 2);
  fmtDayClock(now, buf, sizeof buf);
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextDatum(textdatum_t::middle_center);
  g.drawString(buf, 450, HEADER_H / 2);
  if (v.idle) {
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(C_DIM));
    g.setTextDatum(textdatum_t::middle_right);
    g.drawString("Hold for menu", W - 20, HEADER_H / 2);
  } else {
    g.fillRoundRect(SCHED_CLOSE.x, SCHED_CLOSE.y, SCHED_CLOSE.w, SCHED_CLOSE.h, 8, rgb(0x3A3A48));
    g.setTextColor(rgb(C_TEXT));
    g.drawString("CLOSE", SCHED_CLOSE.x + SCHED_CLOSE.w / 2, HEADER_H / 2);
    if (pages > 1) {
      snprintf(buf, sizeof buf, "MORE %d/%d", page + 1, pages);
      g.setFont(&fonts::FreeSansBold9pt7b);
      g.setTextColor(rgb(C_DIM));
      g.setTextDatum(textdatum_t::middle_right);
      g.drawString(buf, SCHED_CLOSE.x - 14, HEADER_H / 2);
    }
  }

  if (v.count == 0) {
    g.setTextDatum(textdatum_t::middle_center);
    g.setFont(&fonts::FreeSans12pt7b);
    g.setTextColor(rgb(C_DIM));
    g.drawString("No sessions in the next 10 days (or schedule not loaded yet)", W / 2, H / 2);
    g.pushSprite(lcd, 0, 0);
    return;
  }

  // Hero card
  constexpr int HERO_Y = SCHED_HERO_Y, HERO_H = SCHED_HERO_H;
  if (hero >= 0) {
    const ScheduledSession& s = v.sessions[hero];
    const uint32_t accent = v.accents[s.series];
    const bool onTrack = s.start <= now;
    g.fillRoundRect(20, HERO_Y, W - 40, HERO_H, 10, rgb(C_ROW_B));
    g.fillRect(20, HERO_Y, 8, HERO_H, rgb(accent));
    g.setTextDatum(textdatum_t::top_left);
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(C_DIM));
    g.drawString(onTrack ? "ON TRACK NOW" : "NEXT", 44, HERO_Y + 10);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(rgb(accent));
    g.drawString(v.seriesNames[s.series], 44, HERO_Y + 30);
    const int nameX = 44 + g.textWidth(v.seriesNames[s.series]) + 14;
    g.setTextColor(rgb(C_TEXT));
    g.drawString(s.name, nameX, HERO_Y + 30);
    fmtDayClock(s.start, buf, sizeof buf);
    char line[64];
    char endClock[12];
    fmtClock(s.end, endClock, sizeof endClock);
    snprintf(line, sizeof line, "%s - %s%s", buf, endClock, s.confirmed ? "" : "  (time to be confirmed)");
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(C_DIM));
    g.drawString(line, 44, HERO_Y + 60);

    uint32_t c;
    sessionStatus(s, now, buf, sizeof buf, &c);
    if (!onTrack && strncmp(buf, "in ", 3) == 0) memmove(buf, buf + 3, strlen(buf + 3) + 1);  // "1h 12m"
    g.setTextDatum(textdatum_t::middle_right);
    g.setFont(&fonts::FreeSansBold18pt7b);
    g.setTextColor(rgb(onTrack ? C_PB : C_TEXT));
    g.drawString(buf, W - 44, HERO_Y + HERO_H / 2);
  }

  // Session list with day headings
  int y = SCHED_LIST_Y;
  constexpr int ROW = SCHED_ROW, DAY = SCHED_DAY;
  int lastDay = -1;
  for (int i = listStart; i < listEnd; i++) {
    const ScheduledSession& s = v.sessions[i];
    const int day = localDay(s.start);
    if (day != lastDay) {
      fmtDayHeader(s.start, buf, sizeof buf);
      g.setTextDatum(textdatum_t::bottom_left);
      g.setFont(&fonts::FreeSansBold9pt7b);
      g.setTextColor(rgb(C_DIM));
      g.drawString(buf, 20, y + DAY - 4);
      y += DAY;
      lastDay = day;
    }

    const uint32_t accent = v.accents[s.series];
    const bool done = now >= s.end;
    if (i == hero) g.fillRoundRect(12, y, W - 24, ROW, 6, rgb(C_ROW_B));
    const int cy = y + ROW / 2;
    fmtClock(s.start, buf, sizeof buf);
    g.setTextDatum(textdatum_t::middle_right);
    g.setFont(&fonts::FreeSansBold9pt7b);
    g.setTextColor(rgb(done ? C_DIM : C_TEXT));
    g.drawString(buf, 104, cy);
    g.fillRect(118, y + 5, 5, ROW - 10, rgb(done ? C_DIM : accent));
    g.setTextDatum(textdatum_t::middle_left);
    g.setTextColor(rgb(done ? C_DIM : accent));
    g.drawString(v.seriesNames[s.series], 134, cy);
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(done ? C_DIM : C_TEXT));
    g.drawString(s.name, 350, cy);
    uint32_t c;
    sessionStatus(s, now, buf, sizeof buf, &c);
    g.setTextDatum(textdatum_t::middle_right);
    g.setFont(&fonts::FreeSansBold9pt7b);
    g.setTextColor(rgb(c));
    g.drawString(buf, W - 24, cy);
    y += ROW;
  }

  g.pushSprite(lcd, 0, 0);
}
