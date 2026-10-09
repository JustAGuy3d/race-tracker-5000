#pragma once
// Parser for the Porsche Carrera Cup North America live-timing state (Al Kamel data
// relayed by Porsche's live-timing service). Each poll returns the full state.
// Free of Arduino headers so it can be unit-tested on a PC.

#include <ArduinoJson.h>

#include "f1state.h"

namespace pcc {

void applyState(F1State& st, JsonObjectConst root);

// True when the feed reports a session in progress (poll fast), false when idle.
bool sessionRunning(const F1State& st);

// True when a Carrera Cup session (not another series at the same event) is
// running and hasn't taken the chequered flag. Used by auto-switch.
bool carreraRunning(JsonObjectConst root);

}  // namespace pcc
