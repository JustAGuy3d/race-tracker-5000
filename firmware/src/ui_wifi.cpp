#include "ui_wifi.h"

#include <cstring>

#include "ui_theme.h"

using namespace theme;

namespace {

constexpr int W = 800, H = 480, HEADER_H = 54;

LGFX* lcd = nullptr;
LGFX_Sprite canvas;  // full-screen buffer so redraws don't flicker

struct Rect {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

LGFX_Sprite& begin() {
  if (!canvas.getBuffer()) {
    canvas.setPsram(true);
    canvas.setColorDepth(16);
    canvas.createSprite(W, H);
  }
  canvas.fillScreen(rgb(C_BG));
  return canvas;
}

void button(LGFX_Sprite& g, const Rect& r, const char* label, uint32_t bg = C_BUTTON, uint32_t fg = C_TEXT,
            const lgfx::IFont* font = &fonts::FreeSansBold12pt7b) {
  g.fillRoundRect(r.x, r.y, r.w, r.h, 8, rgb(bg));
  g.setFont(font);
  g.setTextColor(rgb(fg));
  g.setTextDatum(textdatum_t::middle_center);
  g.drawString(label, r.x + r.w / 2, r.y + r.h / 2 + 1);
}

void headerBar(LGFX_Sprite& g, const char* title) {
  g.fillRect(0, 0, W, HEADER_H, rgb(C_HEADER));
  g.setTextDatum(textdatum_t::middle_left);
  g.setFont(&fonts::FreeSansBold18pt7b);
  g.setTextColor(rgb(C_TEXT));
  g.drawString(title, 20, HEADER_H / 2);
}

// Text shortened with "..." to fit `maxW`.
void drawFitted(LGFX_Sprite& g, const char* text, int x, int y, int maxW) {
  if (g.textWidth(text) <= maxW) {
    g.drawString(text, x, y);
    return;
  }
  char buf[48];
  snprintf(buf, sizeof buf, "%s", text);
  for (size_t n = strlen(buf); n > 0; n--) {
    buf[n] = 0;
    char tmp[52];
    snprintf(tmp, sizeof tmp, "%s...", buf);
    if (g.textWidth(tmp) <= maxW) {
      g.drawString(tmp, x, y);
      return;
    }
  }
}

// ---- Network list layout
const Rect CLOSE_BTN = {640, 8, 140, 38};
const Rect RESCAN_BTN = {480, 8, 148, 38};
constexpr int LIST_X = 16, LIST_W = 488, LIST_Y = 82, NET_ROW = 44, MAX_NET_ROWS = 8;
constexpr int SAVED_X = 520, SAVED_W = W - SAVED_X - 16, SAVED_ROW = 48;

int visibleNets(const WifiListView& v) { return v.netCount < MAX_NET_ROWS ? v.netCount : MAX_NET_ROWS; }
Rect netRect(int i) { return {LIST_X, LIST_Y + i * NET_ROW, LIST_W, NET_ROW - 4}; }
Rect otherRect(const WifiListView& v) { return netRect(visibleNets(v)); }
Rect forgetRect(int j) { return {W - 16 - 80, LIST_Y + j * SAVED_ROW + 4, 80, SAVED_ROW - 12}; }

void signalBars(LGFX_Sprite& g, int x, int yBottom, int rssi) {
  const int level = rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -75 ? 2 : 1;
  for (int b = 0; b < 4; b++) {
    const int h = 6 + b * 5;
    g.fillRect(x + b * 7, yBottom - h, 5, h, rgb(b < level ? C_TEXT : C_BUTTON));
  }
}

// ---- Keyboard layout
struct Key {
  Rect r;
  int code;  // character, or a KeyHit
  const char* label;
};

constexpr int KB_Y = 150, KB_ROW_H = 74, KB_GAP = 8, KEY_W = 68;

// Returns the keys for the current mode.
int layoutKeys(const KeyboardView& v, Key* keys) {
  static const char* letters[3] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  static const char* sym1[3] = {"1234567890", "-/:;()$&@", ".,?!'\"_"};
  static const char* sym2[3] = {"[]{}#%^*+=", "\\|~<>`", ".,?!'\"_"};
  const char** rows = v.mode == KB_LETTERS ? letters : v.mode == KB_SYMBOLS ? sym1 : sym2;
  static char labels[40][2];
  int n = 0;

  auto charRow = [&](const char* chars, int x0, int y) {
    for (int i = 0; chars[i]; i++) {
      char c = chars[i];
      if (v.mode == KB_LETTERS && v.shift) c = static_cast<char>(toupper(c));
      labels[n][0] = c;
      labels[n][1] = 0;
      keys[n] = {{x0 + i * (KEY_W + KB_GAP), y, KEY_W, KB_ROW_H}, static_cast<unsigned char>(c), labels[n]};
      n++;
    }
  };
  auto centred = [](const char* chars) {
    const int len = strlen(chars);
    return (W - (len * KEY_W + (len - 1) * KB_GAP)) / 2;
  };

  int y = KB_Y;
  charRow(rows[0], centred(rows[0]), y);
  y += KB_ROW_H + KB_GAP;
  charRow(rows[1], centred(rows[1]), y);
  y += KB_ROW_H + KB_GAP;
  // Row 3: [shift / page] chars [delete]
  const int len3 = strlen(rows[2]);
  const int wide = (W - 2 * 24 - len3 * KEY_W - (len3 + 1) * KB_GAP) / 2;
  keys[n++] = {{24, y, wide, KB_ROW_H}, KEY_SHIFT,
               v.mode == KB_LETTERS ? "SHIFT" : v.mode == KB_SYMBOLS ? "#+=" : "123"};
  charRow(rows[2], 24 + wide + KB_GAP, y);
  keys[n++] = {{W - 24 - wide, y, wide, KB_ROW_H}, KEY_DEL, "DEL"};
  y += KB_ROW_H + KB_GAP;
  // Row 4: [ABC/123] [space] [OK]
  keys[n++] = {{24, y, 120, KB_ROW_H}, KEY_MODE, v.mode == KB_LETTERS ? "123" : "ABC"};
  keys[n++] = {{24 + 120 + KB_GAP, y, W - 48 - 120 - 200 - 2 * KB_GAP, KB_ROW_H}, ' ', "SPACE"};
  keys[n++] = {{W - 24 - 200, y, 200, KB_ROW_H}, KEY_OK, v.okLabel};
  return n;
}

const Rect KB_CANCEL = {640, 8, 140, 38};

}  // namespace

void uiWifiBegin(LGFX* display) { lcd = display; }

void uiWifiList(const WifiListView& v) {
  LGFX_Sprite& g = begin();
  headerBar(g, "WI-FI");
  button(g, RESCAN_BTN, "RESCAN");
  button(g, CLOSE_BTN, "CLOSE");

  g.setTextDatum(textdatum_t::bottom_left);
  g.setFont(&fonts::FreeSans9pt7b);
  g.setTextColor(rgb(C_DIM));
  g.drawString("NEARBY NETWORKS (2.4 GHz)", LIST_X, LIST_Y - 8);
  g.drawString("SAVED ON THIS DISPLAY", SAVED_X, LIST_Y - 8);

  if (v.scanning) {
    g.setTextDatum(textdatum_t::middle_center);
    g.setFont(&fonts::FreeSans12pt7b);
    g.drawString("Scanning...", LIST_X + LIST_W / 2, 250);
  } else {
    for (int i = 0; i < visibleNets(v); i++) {
      const WifiNet& n = v.nets[i];
      const Rect r = netRect(i);
      const bool connected = v.connectedSsid[0] && strcmp(n.ssid, v.connectedSsid) == 0;
      bool saved = false;
      for (int j = 0; j < v.savedCount && !saved; j++) saved = v.saved[j] == n.ssid;
      g.fillRoundRect(r.x, r.y, r.w, r.h, 8, rgb(connected ? 0x1E3A2A : C_ROW_B));
      g.setTextDatum(textdatum_t::middle_left);
      g.setFont(&fonts::FreeSansBold12pt7b);
      g.setTextColor(rgb(C_TEXT));
      drawFitted(g, n.ssid, r.x + 14, r.y + r.h / 2, 280);
      const char* tag = connected ? "CONNECTED" : saved ? "SAVED" : n.secure ? "" : "OPEN";
      g.setTextDatum(textdatum_t::middle_right);
      g.setFont(&fonts::FreeSansBold9pt7b);
      g.setTextColor(rgb(connected ? C_PB : C_DIM));
      g.drawString(tag, r.x + r.w - 50, r.y + r.h / 2);
      signalBars(g, r.x + r.w - 40, r.y + r.h - 8, n.rssi);
    }
    if (!v.netCount) {
      g.setTextDatum(textdatum_t::top_left);
      g.setFont(&fonts::FreeSans12pt7b);
      g.setTextColor(rgb(C_DIM));
      g.drawString("No networks found. Tap RESCAN.", LIST_X, LIST_Y + 8);
    }
    const Rect o = otherRect(v);
    g.drawRoundRect(o.x, o.y, o.w, o.h, 8, rgb(C_BUTTON));
    g.setTextDatum(textdatum_t::middle_left);
    g.setFont(&fonts::FreeSans12pt7b);
    g.setTextColor(rgb(C_DIM));
    g.drawString("Other (hidden network)...", o.x + 14, o.y + o.h / 2);
  }

  for (int j = 0; j < v.savedCount; j++) {
    const Rect f = forgetRect(j);
    g.setTextDatum(textdatum_t::middle_left);
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(C_TEXT));
    drawFitted(g, v.saved[j].c_str(), SAVED_X, f.y + f.h / 2, SAVED_W - f.w - 8);
    button(g, f, "FORGET", C_BUTTON, C_TEXT, &fonts::FreeSansBold9pt7b);
  }
  if (!v.savedCount) {
    g.setTextDatum(textdatum_t::top_left);
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(rgb(C_DIM));
    g.drawString("None yet. Networks in config.h", SAVED_X, LIST_Y + 4);
    g.drawString("are always tried as well.", SAVED_X, LIST_Y + 26);
  }
  g.setTextDatum(textdatum_t::bottom_left);
  g.setFont(&fonts::FreeSans9pt7b);
  g.setTextColor(rgb(C_DIM));
  g.drawString("Tap a network to connect.", SAVED_X, H - 16);
  g.pushSprite(lcd, 0, 0);
}

int uiWifiListHit(const WifiListView& v, int x, int y) {
  if (CLOSE_BTN.contains(x, y)) return WL_CLOSE;
  if (RESCAN_BTN.contains(x, y)) return WL_RESCAN;
  if (v.scanning) return WL_NONE;
  for (int i = 0; i < visibleNets(v); i++)
    if (netRect(i).contains(x, y)) return i;
  if (otherRect(v).contains(x, y)) return WL_OTHER;
  for (int j = 0; j < v.savedCount; j++)
    if (forgetRect(j).contains(x, y)) return WL_FORGET + j;
  return WL_NONE;
}

void uiKeyboard(const KeyboardView& v) {
  LGFX_Sprite& g = begin();
  g.fillRect(0, 0, W, HEADER_H, rgb(C_HEADER));
  g.setTextDatum(textdatum_t::middle_left);
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextColor(rgb(C_TEXT));
  drawFitted(g, v.title, 20, HEADER_H / 2, KB_CANCEL.x - 40);
  button(g, KB_CANCEL, "CANCEL");

  // Text field (monospace so l/1/I and O/0 are easy to tell apart); shows the
  // end of the text if it's too long.
  g.fillRoundRect(20, 66, W - 40, 52, 8, rgb(C_ROW_B));
  g.setFont(&fonts::FreeMonoBold12pt7b);
  g.setTextColor(rgb(C_TEXT));
  g.setTextDatum(textdatum_t::middle_left);
  const char* shown = v.text;
  while (*shown && g.textWidth(shown) > W - 90) shown++;
  g.drawString(shown, 36, 92);
  g.fillRect(36 + g.textWidth(shown) + 2, 80, 3, 26, rgb(C_PB));  // cursor

  g.setFont(&fonts::FreeSans9pt7b);
  g.setTextColor(rgb(v.statusColour));
  g.drawString(v.status, 22, 134);

  Key keys[48];
  const int n = layoutKeys(v, keys);
  for (int i = 0; i < n; i++) {
    const Key& k = keys[i];
    const bool special = k.code <= 0 || k.code == ' ';
    uint32_t bg = special ? C_BUTTON : C_ROW_B;
    if (k.code == KEY_OK) bg = 0x1E6B3C;
    if (k.code == KEY_SHIFT && v.mode == KB_LETTERS && v.shift) bg = 0x5A5A70;
    button(g, k.r, k.label, bg, C_TEXT, special ? &fonts::FreeSansBold9pt7b : &fonts::FreeSansBold12pt7b);
  }
  g.pushSprite(lcd, 0, 0);
}

int uiKeyboardHit(const KeyboardView& v, int x, int y) {
  if (KB_CANCEL.contains(x, y)) return KEY_CANCEL;
  Key keys[48];
  const int n = layoutKeys(v, keys);
  for (int i = 0; i < n; i++)
    if (keys[i].r.contains(x, y)) return keys[i].code;
  return KEY_NONE;
}
