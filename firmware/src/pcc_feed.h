#pragma once
// Porsche Carrera Cup North America live timing. Polls the same JSON state that
// racing.porsche.com's live-timing page polls (Al Kamel timing data).

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "source.h"

class PccFeed : public TimingSource {
public:
  const char* title() const override { return "PORSCHE CARRERA CUP"; }
  const char* menuName() const override { return "Porsche Carrera Cup"; }
  uint32_t accent() const override { return 0xC0A062; }
  const char* driverHeading() const override { return "CAR"; }

  void start(F1State* state) override;
  void stop() override;
  void loop() override;
  bool live() const override;
  bool running(const F1State&) const override { return carreraRunning_; }
  int probe() override;

private:
  bool poll();

  F1State* state_ = nullptr;
  WiFiClientSecure client_;
  HTTPClient http_;
  bool     active_ = false;
  bool     carreraRunning_ = false;
  uint32_t lastPollMs_ = 0;
  uint32_t lastOkMs_ = 0;
};
