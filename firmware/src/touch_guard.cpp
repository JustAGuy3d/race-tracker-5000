#include "touch_guard.h"

#include <Arduino.h>

namespace {

constexpr int I2C_PORT = 1, PIN_SDA = 19, PIN_SCL = 20;
constexpr int PCA9557 = 0x18;      // v3.0 boards: IO0 = touch reset, IO1 = touch INT
constexpr uint32_t FREQ = 400000;
constexpr uint32_t CHECK_MS = 5000;

LGFX* lcd = nullptr;
bool isV3 = false;
uint32_t recoveries = 0;

// The expander is only ever READ (to detect a v3.0 board), never written.
//
// Earlier versions pulsed the GT911 reset through PCA9557 IO0 (the sequence in
// Elecrow's V3.0 hardware guide); touch later stopped answering on two boards.
// It's not proven that the pulse caused it, but Elecrow's own V3.0 example
// leaves the expander alone and lets the GT911 start by itself, so this does
// too. A stuck touch controller can be recovered by holding BOOT + RESET for
// about 10 s (see README).
void detectExpander() {
  lgfx::i2c::init(I2C_PORT, PIN_SDA, PIN_SCL);
  uint8_t reg = 0x03, cfg = 0;
  isV3 = lgfx::i2c::transactionWriteRead(I2C_PORT, PCA9557, &reg, 1, &cfg, 1, FREQ).has_value();
}

// True if the controller answers a read of its product ID register.
bool answers() {
  lgfx::ITouch* t = lcd ? lcd->touch() : nullptr;
  if (!t) return false;
  const uint8_t reg[2] = {0x81, 0x40};
  uint8_t id[4] = {0};
  return lgfx::i2c::transactionWriteRead(I2C_PORT, t->config().i2c_addr, reg, 2, id, 4, FREQ).has_value();
}

bool bringUp(int attempts = 6) {
  lgfx::ITouch* t = lcd ? lcd->touch() : nullptr;
  if (!t) return false;
  for (int attempt = 1; attempt <= attempts; attempt++) {
    if (attempt > 1) delay(80 * attempt);  // a cold controller needs longer than a warm one
    if (t->init() && answers()) {
      Serial.printf("[touch] controller ready at 0x%02X (attempt %d)\n", t->config().i2c_addr, attempt);
      return true;
    }
  }
  return false;
}

}  // namespace

void touchWakeBoard() {
  detectExpander();
  if (isV3) Serial.println("[board] PCA9557 found (v3.0 board)");
}

void touchBegin(LGFX* display) {
  lcd = display;
  if (answers()) {
    Serial.printf("[touch] controller ready at 0x%02X\n", lcd->touch()->config().i2c_addr);
    return;
  }
  Serial.println("[touch] controller did not start with the display, retrying");
  if (!bringUp()) Serial.println("[touch] NOT FOUND on the I2C bus - touch will not work (retrying in the background)");
}

void touchWatch() {
  static uint32_t lastCheck = 0;
  static int misses = 0;
  if (millis() - lastCheck < CHECK_MS) return;
  lastCheck = millis();
  if (answers()) {
    misses = 0;
    return;
  }
  if (++misses < 2) return;  // one missed reply can be a glitch
  // Say so once, then retry quietly: every check at first, later every 30 s
  // (each attempt blocks the display for a moment).
  if (misses == 2) Serial.println("[touch] controller stopped answering, trying to recover it");
  if (misses > 6 && misses % 6 != 0) return;
  if (bringUp(1)) {  // one quick try: this runs on the display loop
    recoveries++;
    misses = 0;
  }
}

void touchReport() {
  lgfx::ITouch* t = lcd ? lcd->touch() : nullptr;
  int32_t x = -1, y = -1;
  const bool down = lcd && lcd->getTouch(&x, &y);
  Serial.printf("[touch] address 0x%02X, answering: %s, v3 board: %s, recoveries: %u, finger: %s",
                t ? t->config().i2c_addr : 0, answers() ? "yes" : "NO", isV3 ? "yes" : "no", (unsigned)recoveries,
                down ? "down" : "up");
  if (down) Serial.printf(" at %ld,%ld", (long)x, (long)y);
  Serial.println();
  // Everything that answers on the bus (expect the expander at 0x18 and the
  // touch controller at 0x5D or 0x14).
  Serial.print("[touch] I2C devices:");
  for (int addr = 0x08; addr < 0x78; addr++) {
    for (uint32_t freq : {100000u, 400000u}) {
      if (lgfx::i2c::transactionWrite(I2C_PORT, addr, nullptr, 0, freq).has_value()) {
        Serial.printf(" 0x%02X@%uk", addr, (unsigned)(freq / 1000));
        break;
      }
    }
  }
  Serial.println();
  uint8_t reg = 0, in = 0, out = 0, cfg = 0;
  reg = 0x00; lgfx::i2c::transactionWriteRead(I2C_PORT, PCA9557, &reg, 1, &in, 1, FREQ);
  reg = 0x01; lgfx::i2c::transactionWriteRead(I2C_PORT, PCA9557, &reg, 1, &out, 1, FREQ);
  reg = 0x03; lgfx::i2c::transactionWriteRead(I2C_PORT, PCA9557, &reg, 1, &cfg, 1, FREQ);
  Serial.printf("[touch] expander: input 0x%02X, output 0x%02X, config 0x%02X\n", in, out, cfg);
}
