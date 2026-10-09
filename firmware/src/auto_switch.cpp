#include "auto_switch.h"

#include <Arduino.h>
#include <WiFi.h>

#include "schedule.h"

namespace {

constexpr int MAX_SOURCES = 4;
constexpr uint32_t GAP_MS = 20000;  // between probes; each other series ~every 40 s

TimingSource* const* sources = nullptr;
int count = 0;
volatile bool enabled = false;
volatile int active = -1;
volatile int8_t states[MAX_SOURCES] = {-1, -1, -1, -1};

void task(void*) {
  int next = 0;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    scheduleTick();  // runs whether or not auto-switch is on
    if (!enabled) {
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }
    const int i = next;
    next = (next + 1) % count;
    if (i == active) continue;

    const int r = sources[i]->probe();
    if (r != states[i])
      Serial.printf("[auto] %s: %s\n", sources[i]->title(),
                    r > 0 ? "session running" : r == 0 ? "no session" : "check failed");
    states[i] = static_cast<int8_t>(r);
    vTaskDelay(pdMS_TO_TICKS(GAP_MS));
  }
}

}  // namespace

void autoProbeBegin(TimingSource* const* s, int n) {
  sources = s;
  count = n < MAX_SOURCES ? n : MAX_SOURCES;
  // TLS handshakes need a deep stack.
  xTaskCreatePinnedToCore(task, "autoProbe", 16384, nullptr, 1, nullptr, 0);
}

void autoProbeSetEnabled(bool e) { enabled = e; }

void autoProbeSetActive(int index) {
  active = index;
  if (index >= 0 && index < MAX_SOURCES) states[index] = -1;
}

int autoProbeState(int index) { return index >= 0 && index < count ? states[index] : -1; }
