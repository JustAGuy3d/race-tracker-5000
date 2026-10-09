#pragma once
// Keeps the GT911 touch controller working.
//
// LovyanGFX only tries to reach the controller for a few milliseconds during
// lcd.init(); if it isn't ready (a cold power-on is slower than a reset), touch
// stays dead until the next restart. This module resets the controller, retries
// until it answers, and re-does that at runtime if it ever stops answering.

#include "display.h"

// Before lcd.init(): detects a CrowPanel 7.0" v3.0 board. Never writes to its
// PCA9557 I/O expander (see touch_guard.cpp for why).
void touchWakeBoard();

// After lcd.init(): makes sure touch is up, retrying with a reset if needed.
void touchBegin(LGFX* lcd);

// From loop(): checks the controller every few seconds and recovers it.
void touchWatch();

// Prints the controller's state to Serial.
void touchReport();
