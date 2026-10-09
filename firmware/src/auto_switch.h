#pragma once
// Background work on core 0, so the display (core 1) never waits on it:
// refreshes the session schedule, and for auto-switch probes each series that
// isn't on screen in turn.

#include "source.h"

void autoProbeBegin(TimingSource* const* sources, int count);
void autoProbeSetEnabled(bool enabled);
void autoProbeSetActive(int index);  // the on-screen series is not probed
int autoProbeState(int index);       // 1 running, 0 idle, -1 unknown
