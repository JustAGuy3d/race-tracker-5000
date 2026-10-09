#pragma once
// On-screen Wi-Fi setup: the network list and a touch keyboard.

#include <Arduino.h>

#include "display.h"
#include "wifi_store.h"

void uiWifiBegin(LGFX* lcd);

// ---- Network list -------------------------------------------------------------
struct WifiListView {
  const WifiNet* nets;
  int            netCount;
  const String*  saved;
  int            savedCount;
  const char*    connectedSsid;  // "" when not connected
  bool           scanning;
};

// network i -> i; saved network j's FORGET -> WL_FORGET + j
enum WifiListHit { WL_NONE = -1, WL_CLOSE = 1000, WL_RESCAN = 1001, WL_OTHER = 1002, WL_FORGET = 2000 };

void uiWifiList(const WifiListView& v);
int uiWifiListHit(const WifiListView& v, int x, int y);

// ---- Keyboard ---------------------------------------------------------------------
enum KeyboardMode { KB_LETTERS = 0, KB_SYMBOLS = 1, KB_SYMBOLS2 = 2 };

struct KeyboardView {
  const char* title;         // e.g. "Password for MyHotspot"
  const char* text;          // typed so far
  int         mode;          // KeyboardMode
  bool        shift;
  const char* status;        // hint or error under the text field
  uint32_t    statusColour;  // 0xRRGGBB
  const char* okLabel;       // "CONNECT" / "NEXT"
};

// > 0: a character to type; otherwise one of these
enum KeyHit { KEY_NONE = 0, KEY_SHIFT = -2, KEY_MODE = -3, KEY_DEL = -4, KEY_OK = -5, KEY_CANCEL = -6 };

void uiKeyboard(const KeyboardView& v);
int uiKeyboardHit(const KeyboardView& v, int x, int y);
