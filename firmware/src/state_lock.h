#pragma once
// Guards the shared F1State: the network task (core 0) writes it as feed data
// arrives, the display loop (core 1) reads it while drawing. Hold the lock only
// for the update or the draw itself, never across network I/O.

void stateLockInit();  // once, from setup(), before the network task starts
void stateLock();
void stateUnlock();

struct StateGuard {
  StateGuard() { stateLock(); }
  ~StateGuard() { stateUnlock(); }
  StateGuard(const StateGuard&) = delete;
  StateGuard& operator=(const StateGuard&) = delete;
};
