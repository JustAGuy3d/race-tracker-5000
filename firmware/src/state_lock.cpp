#include "state_lock.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
SemaphoreHandle_t mutex = nullptr;
}

void stateLockInit() { mutex = xSemaphoreCreateRecursiveMutex(); }

void stateLock() {
  if (mutex) xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
}

void stateUnlock() {
  if (mutex) xSemaphoreGiveRecursive(mutex);
}
