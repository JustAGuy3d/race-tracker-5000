#pragma once
// F1 Academy live timing: ASP.NET SignalR 2 ("Streaming" hub) client.
//
//   GET  /streaming/negotiate?clientProtocol=2.1&connectionData=...   -> ConnectionToken
//   WSS  /streaming/connect?transport=webSockets&connectionToken=...
//   GET  /streaming/start?...
//   -> {"H":"Streaming","M":"JoinFeeds","A":[series,[feeds]],"I":0}
//   -> {"H":"Streaming","M":"GetData2","A":[series,[feeds]],"I":1}   <- {"R":{...},"I":"1"}
//   <- {"C":"...","M":[{"H":"Streaming","M":"datafeed","A":[...]}]}  (pushes)
//   <- {}                                                              (keep-alive)

#include <Arduino.h>
#include <WebSocketsClient.h>

#include "psram.h"
#include "source.h"

// WebSocketsClient::begin() leaks the previous TLS client when called again, so a
// reconnect with a new connection token only swaps the URL.
class ReconnectableWsClient : public WebSocketsClient {
public:
  void setUrl(const String& url) { _client.cUrl = url; }
};

class FaFeed : public TimingSource {
public:
  const char* title() const override { return "F1 ACADEMY"; }
  const char* menuName() const override { return "F1 Academy"; }
  uint32_t accent() const override { return 0xD6247D; }

  void start(F1State* state) override;
  void stop() override;
  void loop() override;
  bool live() const override { return phase_ == Phase::Live; }
  bool running(const F1State& state) const override;
  int probe() override;

private:
  enum class Phase { Negotiate, Connecting, Starting, Live };

  static bool negotiate(String& token, String& query);
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  void processMessage(const char* msg, size_t len);
  void subscribe();
  void restart(const char* why);

  ReconnectableWsClient ws_;
  ReconnectableWsClient probeWs_;  // used only by probe(), from the background task
  bool probeBegun_ = false;
  F1State* state_ = nullptr;
  MessageBuffer buf_;
  String query_;  // shared query string for connect/start

  bool     active_ = false;
  bool     begun_ = false;
  Phase    phase_ = Phase::Negotiate;
  uint32_t phaseMs_ = 0;
  uint32_t lastTryMs_ = 0;
  uint32_t lastRxMs_ = 0;
};
