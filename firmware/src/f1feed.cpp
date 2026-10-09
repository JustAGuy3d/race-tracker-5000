#include "f1feed.h"

#include "config.h"
#include "net_util.h"

namespace {

constexpr char RS = 0x1e;  // SignalR record separator
constexpr uint32_t PING_EVERY_MS    = 15000;
constexpr uint32_t RX_TIMEOUT_MS    = 45000;         // server pings every 15 s
constexpr uint32_t IDLE_REFRESH_MS  = 15 * 60000;    // reconnect when no session is running

const char* const TOPICS[] = {
    "DriverList", "TimingData", "TimingAppData", "SessionInfo",
    "SessionStatus", "LapCount", "TrackStatus", "ExtrapolatedClock",
};

}  // namespace

void F1Feed::start(F1State* state) {
  state_ = state;
  if (begun_) return;  // after stop(), ws_.loop() reconnects on its own
  begun_ = true;
  ws_.onEvent([this](WStype_t t, uint8_t* p, size_t l) { onEvent(t, p, l); });
  ws_.setReconnectInterval(5000);
  // Empty fingerprint => TLS without certificate verification (public, read-only data).
  ws_.beginSSL(F1_HOST, 443, F1_PATH, "", "");
}

void F1Feed::stop() {
  wsUp_ = subscribed_ = false;
  ws_.disconnect();
}

void F1Feed::loop() {
  ws_.loop();
  if (!wsUp_) return;

  uint32_t now = millis();
  if (wantReconnect_) {
    wantReconnect_ = false;
    reconnect("session changed");
    return;
  }
  if (now - lastRxMs_ > RX_TIMEOUT_MS) {
    reconnect("receive timeout");
    return;
  }
  if (now - connectedMs_ > IDLE_REFRESH_MS && strcmp(state_->session.status, "Started") != 0) {
    reconnect("idle refresh");
    return;
  }
  if (subscribed_ && now - lastPingMs_ > PING_EVERY_MS) {
    lastPingMs_ = now;
    char ping[] = "{\"type\":6}\x1e";
    ws_.sendTXT(ping);
  }
}

void F1Feed::reconnect(const char* why) {
  Serial.printf("[feed] reconnecting: %s\n", why);
  subscribed_ = false;
  ws_.disconnect();  // WebSocketsClient reconnects on its own after the interval
}

void F1Feed::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[feed] connected");
      wsUp_ = true;
      subscribed_ = false;
      connectedMs_ = lastRxMs_ = lastPingMs_ = millis();
      buf_.clear();
      sendHandshake();
      break;
    case WStype_DISCONNECTED:
      if (wsUp_) Serial.println("[feed] disconnected");
      wsUp_ = subscribed_ = false;
      buf_.clear();
      break;
    case WStype_TEXT:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      lastRxMs_ = millis();
      if (type == WStype_TEXT || type == WStype_FRAGMENT_TEXT_START) buf_.clear();
      if (!buf_.append(payload, length)) Serial.println("[feed] out of PSRAM, dropping message");
      if (type == WStype_TEXT || type == WStype_FRAGMENT_FIN) processBuffer();
      break;
    case WStype_ERROR:
      Serial.printf("[feed] error: %.*s\n", (int)length, payload ? (char*)payload : "");
      break;
    default:
      break;
  }
}

void F1Feed::processBuffer() {
  char* data = buf_.data();
  size_t start = 0;
  for (size_t i = 0; i < buf_.size(); i++) {
    if (data[i] != RS) continue;
    if (i > start) processRecord(data + start, i - start);
    start = i + 1;
  }
  buf_.clear();
}

void F1Feed::processRecord(const char* rec, size_t len) {
  JsonDocument doc(SpiRamAllocator::instance());
  DeserializationError err = deserializeJson(doc, rec, len);
  if (err) {
    Serial.printf("[feed] JSON error %s (%u bytes)\n", err.c_str(), (unsigned)len);
    return;
  }

  if (doc["type"].isNull()) {  // handshake response: {} or {"error": "..."}
    if (doc["error"].is<const char*>()) {
      Serial.printf("[feed] handshake refused: %s\n", doc["error"].as<const char*>());
      return;
    }
    subscribe();
    return;
  }

  bool keyChanged = false;
  switch (doc["type"].as<int>()) {
    case 1:  // invocation
      if (doc["target"] == "feed") {
        JsonArrayConst args = doc["arguments"];
        keyChanged = state_->applyTopic(args[0], args[1]);
      }
      break;
    case 3:  // completion of Subscribe -> full snapshot
      if (!doc["error"].isNull()) {
        Serial.printf("[feed] subscribe failed: %s\n", doc["error"].as<const char*>());
        break;
      }
      // Session info first so a session change is detected before the rest.
      if (!doc["result"]["SessionInfo"].isNull())
        state_->applyTopic("SessionInfo", doc["result"]["SessionInfo"]);
      for (JsonPairConst kv : doc["result"].as<JsonObjectConst>())
        if (strcmp(kv.key().c_str(), "SessionInfo") != 0)
          state_->applyTopic(kv.key().c_str(), kv.value());
      Serial.printf("[feed] snapshot applied (%u bytes, session %s / %s)\n", (unsigned)len,
                    state_->session.meeting, state_->session.name);
      break;
    case 7:  // close
      Serial.printf("[feed] server closed: %s\n", doc["error"] | "");
      wantReconnect_ = true;
      break;
    default:  // 6 = ping
      break;
  }
  if (keyChanged) wantReconnect_ = true;
}

void F1Feed::sendHandshake() {
  char msg[] = "{\"protocol\":\"json\",\"version\":1}\x1e";
  ws_.sendTXT(msg);
}

void F1Feed::subscribe() {
  JsonDocument doc;
  doc["type"] = 1;
  doc["invocationId"] = String(invocation_++);
  doc["target"] = "Subscribe";
  JsonArray topics = doc["arguments"].add<JsonArray>();
  for (const char* t : TOPICS) topics.add(t);

  String out;
  serializeJson(doc, out);
  out += RS;
  ws_.sendTXT(out);
  subscribed_ = true;
  Serial.println("[feed] subscribed");
}

bool F1Feed::running(const F1State& state) const { return statusIsRunning(state.session.status); }

int F1Feed::probe() {
  String body;
  if (!httpsGet("https://" F1_HOST "/static/SessionInfo.json", body)) return -1;
  const char* json = body.c_str();
  if (strncmp(json, "\xEF\xBB\xBF", 3) == 0) json += 3;  // UTF-8 BOM
  JsonDocument doc;
  if (deserializeJson(doc, json)) return -1;
  return statusIsRunning(doc["SessionStatus"]) ? 1 : 0;
}
