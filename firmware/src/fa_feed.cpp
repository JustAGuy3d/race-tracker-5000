#include "fa_feed.h"

#include "config.h"
#include "fa_parse.h"
#include "net_util.h"
#include "state_lock.h"

namespace {

constexpr uint32_t RETRY_MS = 5000;
constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t RX_TIMEOUT_MS = 40000;  // server keep-alive is every ~7 s

const char* const CONNECTION_DATA = "%5B%7B%22name%22%3A%22Streaming%22%7D%5D";  // [{"name":"Streaming"}]

String urlEncode(const String& s) {
  static const char* hex = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 3);
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}

}  // namespace

void FaFeed::start(F1State* state) {
  state_ = state;
  active_ = true;
  phase_ = Phase::Negotiate;
  lastTryMs_ = 0;
}

void FaFeed::stop() {
  active_ = false;
  phase_ = Phase::Negotiate;
  ws_.disconnect();
}

// Gets a single-use connection token; `query` is the shared query string for
// /connect and /start.
bool FaFeed::negotiate(String& token, String& query) {
  String body;
  if (!httpsGet(String("https://" FA_HOST FA_PATH "/negotiate?clientProtocol=2.1&connectionData=") + CONNECTION_DATA,
                body))
    return false;
  JsonDocument doc;
  if (deserializeJson(doc, body) || !doc["ConnectionToken"].is<const char*>()) return false;
  token = doc["ConnectionToken"].as<const char*>();
  query = String("transport=webSockets&clientProtocol=2.1&connectionToken=") + urlEncode(token) +
          "&connectionData=" + CONNECTION_DATA;
  return true;
}

void FaFeed::loop() {
  if (!active_) return;
  const uint32_t now = millis();

  if (phase_ == Phase::Negotiate) {
    // Don't run ws_.loop() here: it would reconnect with the stale token.
    if (lastTryMs_ && now - lastTryMs_ < RETRY_MS) return;
    lastTryMs_ = now;
    String token;
    if (!negotiate(token, query_)) {
      Serial.println("[fa] negotiate failed, retrying");
      return;
    }
    const String url = String(FA_PATH "/connect?") + query_;
    if (!begun_) {
      begun_ = true;
      ws_.onEvent([this](WStype_t t, uint8_t* p, size_t l) { onEvent(t, p, l); });
      ws_.setReconnectInterval(RETRY_MS);
      ws_.beginSSL(FA_HOST, 443, url.c_str(), "", "");
    } else {
      ws_.setUrl(url);
    }
    phase_ = Phase::Connecting;
    phaseMs_ = now;
  }

  ws_.loop();

  if (phase_ == Phase::Connecting && millis() - phaseMs_ > CONNECT_TIMEOUT_MS) {
    restart("connect timeout");
  } else if (phase_ == Phase::Starting) {
    String body;
    httpsGet(String("https://" FA_HOST FA_PATH "/start?") + query_, body);  // completes the SignalR handshake
    subscribe();
    phase_ = Phase::Live;
    lastRxMs_ = millis();
  } else if (phase_ == Phase::Live && millis() - lastRxMs_ > RX_TIMEOUT_MS) {
    restart("receive timeout");
  }
}

void FaFeed::restart(const char* why) {
  Serial.printf("[fa] reconnecting: %s\n", why);
  ws_.disconnect();
  phase_ = Phase::Negotiate;
  lastTryMs_ = 0;
}

void FaFeed::subscribe() {
  String series = FA_SERIES;
  String join = "{\"H\":\"Streaming\",\"M\":\"JoinFeeds\",\"A\":[\"" + series +
                "\",[\"data\",\"weather\",\"status\",\"time\",\"commentary\",\"racedetails\"]],\"I\":0}";
  String get = "{\"H\":\"Streaming\",\"M\":\"GetData2\",\"A\":[\"" + series +
               "\",[\"data\",\"statsfeed\",\"sessionfeed\",\"trackfeed\",\"timefeed\",\"racedetailsfeed\"]],\"I\":1}";
  ws_.sendTXT(join);
  ws_.sendTXT(get);
  Serial.println("[fa] subscribed");
}

void FaFeed::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[fa] connected");
      buf_.clear();
      if (phase_ == Phase::Connecting) phase_ = Phase::Starting;
      break;
    case WStype_DISCONNECTED:
      if (phase_ != Phase::Negotiate) {
        Serial.println("[fa] disconnected");
        phase_ = Phase::Negotiate;  // tokens are single-use: negotiate again
        lastTryMs_ = 0;
      }
      buf_.clear();
      break;
    case WStype_TEXT:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      lastRxMs_ = millis();
      if (type == WStype_TEXT || type == WStype_FRAGMENT_TEXT_START) buf_.clear();
      if (!buf_.append(payload, length)) Serial.println("[fa] out of PSRAM, dropping message");
      if (type == WStype_TEXT || type == WStype_FRAGMENT_FIN) {
        processMessage(buf_.data(), buf_.size());
        buf_.clear();
      }
      break;
    default:
      break;
  }
}

void FaFeed::processMessage(const char* msg, size_t len) {
  if (len <= 2) return;  // "{}" keep-alive
  JsonDocument doc(SpiRamAllocator::instance());
  DeserializationError err = deserializeJson(doc, msg, len);
  if (err) {
    Serial.printf("[fa] JSON error %s (%u bytes)\n", err.c_str(), (unsigned)len);
    return;
  }
  StateGuard guard;  // the display reads the state from the other core
  if (doc["I"] == "1" && doc["R"].is<JsonObjectConst>()) {
    fa::applySnapshot(*state_, doc["R"]);
    Serial.printf("[fa] snapshot applied (%u bytes, %s / %s)\n", (unsigned)len, state_->session.meeting,
                  state_->session.name);
    return;
  }
  if (!doc["E"].isNull()) {
    Serial.printf("[fa] hub error: %s\n", doc["E"].as<const char*>());
    return;
  }
  for (JsonObjectConst m : doc["M"].as<JsonArrayConst>()) fa::applyMessage(*state_, m["M"], m["A"]);
}

bool FaFeed::running(const F1State& state) const { return statusIsRunning(state.session.status); }

// Opens a short-lived second connection, asks only for the session status and
// hangs up. Runs on the background task, so blocking here is fine.
int FaFeed::probe() {
  String token, query;
  if (!negotiate(token, query)) return -1;
  const String url = String(FA_PATH "/connect?") + query;

  bool connected = false, sent = false, done = false;
  int result = -1;
  String msg;
  probeWs_.onEvent([&](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED: connected = true; break;
      case WStype_DISCONNECTED: done = true; break;
      case WStype_TEXT:
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN: {
        if (type == WStype_TEXT || type == WStype_FRAGMENT_TEXT_START) msg = "";
        msg.concat(reinterpret_cast<const char*>(payload), length);
        if (type == WStype_FRAGMENT_TEXT_START || type == WStype_FRAGMENT) break;
        JsonDocument doc;
        if (!deserializeJson(doc, msg) && doc["I"] == "1") {
          result = statusIsRunning(doc["R"]["sessionfeed"][1]["Value"]) ? 1 : 0;
          done = true;
        }
        break;
      }
      default: break;
    }
  });
  if (!probeBegun_) {
    probeBegun_ = true;
    probeWs_.setReconnectInterval(1000);
    probeWs_.beginSSL(FA_HOST, 443, url.c_str(), "", "");
  } else {
    probeWs_.setUrl(url);
  }

  const uint32_t start = millis();
  while (!done && millis() - start < 15000) {
    probeWs_.loop();
    if (connected && !sent) {
      String body;
      httpsGet(String("https://" FA_HOST FA_PATH "/start?") + query, body);
      String get = String("{\"H\":\"Streaming\",\"M\":\"GetData2\",\"A\":[\"") + FA_SERIES +
                   "\",[\"sessionfeed\"]],\"I\":1}";
      probeWs_.sendTXT(get);
      sent = true;
    }
    delay(10);
  }
  probeWs_.disconnect();
  return result;
}
