#include "pcc_feed.h"

#include "config.h"
#include "net_util.h"
#include "pcc_parse.h"
#include "psram.h"

namespace {
constexpr uint32_t POLL_LIVE_MS = 1000;   // same rate as Porsche's own page
constexpr uint32_t POLL_IDLE_MS = 10000;  // no session running: save mobile data
}  // namespace

void PccFeed::start(F1State* state) {
  state_ = state;
  active_ = true;
  lastPollMs_ = 0;
  client_.setInsecure();  // public, read-only data
  http_.setReuse(true);   // keep the TLS connection open between polls
  http_.setTimeout(5000);
}

void PccFeed::stop() {
  active_ = false;
  http_.end();
  client_.stop();
}

bool PccFeed::live() const { return active_ && lastOkMs_ && millis() - lastOkMs_ < 15000; }

void PccFeed::loop() {
  if (!active_) return;
  const uint32_t interval = pcc::sessionRunning(*state_) ? POLL_LIVE_MS : POLL_IDLE_MS;
  if (lastPollMs_ && millis() - lastPollMs_ < interval) return;
  lastPollMs_ = millis();
  if (poll()) lastOkMs_ = millis();
}

bool PccFeed::poll() {
  if (!http_.begin(client_, PCC_URL)) return false;
  http_.addHeader("x-api-key", PCC_API_KEY);
  const int code = http_.GET();
  if (code != 200) {
    Serial.printf("[pcc] GET -> %d\n", code);
    http_.end();
    client_.stop();  // start over with a fresh connection next time
    return false;
  }
  String body = http_.getString();
  http_.end();

  JsonDocument doc(SpiRamAllocator::instance());
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("[pcc] JSON error %s (%u bytes)\n", err.c_str(), body.length());
    return false;
  }
  pcc::applyState(*state_, doc.as<JsonObjectConst>());
  carreraRunning_ = pcc::carreraRunning(doc.as<JsonObjectConst>());

  static char lastLogged[120] = "";
  char now[sizeof lastLogged];
  snprintf(now, sizeof now, "%s / %s / %s", state_->session.meeting, state_->session.name,
           state_->session.status);
  if (strcmp(now, lastLogged) != 0) {
    strcpy(lastLogged, now);
    const Driver* order[MAX_DRIVERS];
    Serial.printf("[pcc] %s (%d cars, %u bytes)\n", now, state_->sorted(order), body.length());
  }
  return true;
}

int PccFeed::probe() {
  String body;
  if (!httpsGet(PCC_URL, body, "x-api-key", PCC_API_KEY)) return -1;
  JsonDocument doc(SpiRamAllocator::instance());
  if (deserializeJson(doc, body)) return -1;
  return pcc::carreraRunning(doc.as<JsonObjectConst>()) ? 1 : 0;
}
