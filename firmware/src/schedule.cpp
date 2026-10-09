#include "schedule.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"
#include "net_util.h"
#include "psram.h"

namespace {

constexpr uint32_t REFRESH_MS = 3 * 3600 * 1000UL;
constexpr uint32_t RETRY_MS = 5 * 60 * 1000UL;
constexpr time_t KEEP_PAST = 12 * 3600;     // show sessions that ended in the last 12 h
constexpr time_t LOOK_AHEAD = 10 * 86400;  // and the next 10 days

const sched::ManualSession kPorsche[] = {PCC_SCHEDULE{nullptr, nullptr, 0}};

ScheduledSession sessions[MAX_SCHEDULE];
int count = 0;
SemaphoreHandle_t lock = nullptr;  // created by scheduleBegin()
uint32_t lastOkMs = 0, lastTryMs = 0;
bool everLoaded = false;

// Downloads JSON, transparently un-gzipping (api.formula1.com always gzips).
bool fetchJson(const char* url, JsonDocument& doc, const char* hName = nullptr, const char* hValue = nullptr) {
  MessageBuffer raw, plain;
  if (!httpsGetBytes(url, raw, hName, hValue)) return false;
  const auto* bytes = reinterpret_cast<const uint8_t*>(raw.data());
  MessageBuffer* body = &raw;
  if (raw.size() > 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
    if (!gunzip(bytes, raw.size(), plain)) {
      Serial.printf("[sched] gunzip failed for %s\n", url);
      return false;
    }
    body = &plain;
  }
  const DeserializationError err = deserializeJson(doc, body->data(), body->size());
  if (err) Serial.printf("[sched] JSON error %s for %s\n", err.c_str(), url);
  return !err;
}

}  // namespace

void scheduleBegin() { lock = xSemaphoreCreateMutex(); }

static SemaphoreHandle_t getLock() { return lock; }

void scheduleTick() {
  const uint32_t now = millis();
  const uint32_t wait = !everLoaded || lastTryMs > lastOkMs ? RETRY_MS : REFRESH_MS;
  if (lastTryMs && now - lastTryMs < wait) return;
  const time_t utc = time(nullptr);
  if (utc < 1700000000) return;  // clock not synced yet
  lastTryMs = now;

  static ScheduledSession fresh[MAX_SCHEDULE];
  int n = 0;
  const time_t from = utc - KEEP_PAST, to = utc + LOOK_AHEAD;
  bool ok = true;
  int nF1 = 0, nFa = 0, nPcc = 0;
  {
    JsonDocument doc(SpiRamAllocator::instance());
    if (fetchJson(SCHEDULE_F1_URL, doc)) nF1 = sched::addF1(doc.as<JsonObjectConst>(), 0, from, to, fresh, &n, MAX_SCHEDULE);
    else ok = false;
  }
  {
    JsonDocument doc(SpiRamAllocator::instance());
    if (fetchJson(SCHEDULE_FA_URL, doc, "apikey", SCHEDULE_FA_KEY))
      nFa = sched::addFa(doc.as<JsonObjectConst>(), 1, from, to, fresh, &n, MAX_SCHEDULE);
    else ok = false;
  }
  nPcc = sched::addManual(kPorsche, 2, from, to, fresh, &n, MAX_SCHEDULE);
  sched::sortByStart(fresh, n);

  // Keep the old list if everything failed (e.g. no internet for a moment).
  if (ok || n > 0) {
    xSemaphoreTake(getLock(), portMAX_DELAY);
    memcpy(sessions, fresh, sizeof(ScheduledSession) * n);
    count = n;
    xSemaphoreGive(getLock());
    everLoaded = true;
  }
  if (ok) lastOkMs = now;
  Serial.printf("[sched] %s: F1 %d, F1 Academy %d, Porsche %d sessions (past 12 h to next 10 days)\n",
                ok ? "updated" : "partly updated", nF1, nFa, nPcc);
}

int scheduleSnapshot(ScheduledSession* out, int max) {
  xSemaphoreTake(getLock(), portMAX_DELAY);
  const int n = count < max ? count : max;
  memcpy(out, sessions, sizeof(ScheduledSession) * n);
  xSemaphoreGive(getLock());
  return n;
}
