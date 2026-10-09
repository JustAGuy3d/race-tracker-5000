#pragma once
// Parser for the F1 Academy live-timing feed (the F2/F3/F1 Academy "Streaming"
// SignalR 2 hub). Free of Arduino headers so it can be unit-tested on a PC.

#include <ArduinoJson.h>

#include "f1state.h"

namespace fa {

// Result of GetData2: {"data":[t, header, lines], "statsfeed":[t, stats], ...}
void applySnapshot(F1State& st, JsonObjectConst result);

// A pushed hub message, e.g. method "datafeed", args [sentTime, lineNo, feed].
void applyMessage(F1State& st, const char* method, JsonArrayConst args);

}  // namespace fa
