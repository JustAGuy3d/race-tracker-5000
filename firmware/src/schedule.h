#pragma once
// Session schedule for all series: fetched in the background (F1 from Jolpica,
// F1 Academy from api.formula1.com, Porsche from config.h), read by the UI.

#include <ctime>

#include "schedule_parse.h"

// Call once from setup(), before the background task starts.
void scheduleBegin();

// Fetches if due (first call, then every 3 hours; 5 minutes after a failure).
// Blocking: call from the background task only.
void scheduleTick();

// Thread-safe copy of the current schedule, sorted by start time.
int scheduleSnapshot(ScheduledSession* out, int max);
