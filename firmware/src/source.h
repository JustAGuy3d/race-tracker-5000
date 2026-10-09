#pragma once
// A live-timing source (one racing series). Only one is active at a time; all of
// them write into the shared F1State.

#include <cstdint>

#include "f1state.h"

class TimingSource {
public:
  virtual ~TimingSource() = default;

  virtual const char* title() const = 0;           // shown in the header
  virtual uint32_t accent() const = 0;             // 0xRRGGBB header accent
  virtual const char* menuName() const = 0;         // shown on the menu button
  virtual const char* driverHeading() const { return "DRIVER"; }

  virtual void start(F1State* state) = 0;  // begin (or resume) receiving
  virtual void stop() = 0;                 // disconnect; loop() is not called while stopped
  virtual void loop() = 0;
  virtual bool live() const = 0;           // connected and receiving data

  // Auto-switch support.
  // running(): the live feed (in `state`) shows a session in progress.
  virtual bool running(const F1State& state) const = 0;
  // probe(): one-shot check run from the background task while this series is
  // not on screen. 1 = session running, 0 = none, -1 = check failed.
  // Must not touch the live connection.
  virtual int probe() = 0;
};
