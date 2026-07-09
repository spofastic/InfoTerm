#pragma once

#include <Arduino.h>

namespace InfoTermScheduler {

using TaskCallback = void (*)();

struct IntervalTask {
  const char* name;
  uint32_t intervalMs;
  uint32_t lastRunMs;
  TaskCallback callback;
  bool enabled;
};

class Scheduler {
 public:
  Scheduler();

  bool add(const char* name, uint32_t intervalMs, TaskCallback callback, bool enabled = true);
  void tick(uint32_t nowMs = millis());
  void setEnabled(const char* name, bool enabled);
  void reset();

 private:
  static const uint8_t MAX_TASKS = 16;
  IntervalTask tasks[MAX_TASKS];
  uint8_t taskCount;
};

extern Scheduler RuntimeScheduler;

}  // namespace InfoTermScheduler
