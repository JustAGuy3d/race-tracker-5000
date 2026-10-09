#pragma once
// Minimal ASP.NET Core SignalR (JSON protocol) client for F1 live timing.
//
//   wss://livetiming.formula1.com/signalrcore   (negotiate skipped)
//   -> {"protocol":"json","version":1}\x1e      <- {}\x1e
//   -> {"type":1,"invocationId":"n","target":"Subscribe","arguments":[[topics]]}\x1e
//   <- {"type":3,"invocationId":"n","result":{topic: snapshot, ...}}\x1e
//   <- {"type":1,"target":"feed","arguments":[topic, partial, timestamp]}\x1e
//   <> {"type":6}\x1e  (ping)

#include <Arduino.h>
#include <WebSocketsClient.h>

#include "psram.h"
#include "source.h"

class F1Feed : public TimingSource {
public:
  const char* title() const override { return "FORMULA 1"; }
  const char* menuName() const override { return "Formula 1"; }
  uint32_t accent() const override { return 0xE10600; }

  void start(F1State* state) override;
  void stop() override;
  void loop() override;
  bool live() const override { return wsUp_ && subscribed_; }
  bool running(const F1State& state) const override;
  int probe() override;

private:
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  void processBuffer();
  void processRecord(const char* rec, size_t len);
  void sendHandshake();
  void subscribe();
  void reconnect(const char* why);

  WebSocketsClient ws_;
  F1State* state_ = nullptr;
  MessageBuffer buf_;

  bool     begun_ = false;
  bool     wsUp_ = false;
  bool     subscribed_ = false;
  uint32_t invocation_ = 0;
  uint32_t lastRxMs_ = 0;
  uint32_t lastPingMs_ = 0;
  uint32_t connectedMs_ = 0;
  bool     wantReconnect_ = false;
};
