#include "Scheduler.h"
#include <string.h>

namespace InfoTermScheduler {

Scheduler RuntimeScheduler;

Scheduler::Scheduler() : taskCount(0) {}

bool Scheduler::add(const char* name, uint32_t intervalMs, TaskCallback callback, bool enabled) {
  if (!name || !callback || intervalMs == 0 || taskCount >= MAX_TASKS) {
    return false;
  }

  tasks[taskCount] = {name, intervalMs, 0, callback, enabled};
  taskCount++;
  return true;
}

void Scheduler::tick(uint32_t nowMs) {
  for (uint8_t i = 0; i < taskCount; i++) {
    IntervalTask& task = tasks[i];
    if (!task.enabled || !task.callback) {
      continue;
    }

    if (task.lastRunMs == 0 || (uint32_t)(nowMs - task.lastRunMs) >= task.intervalMs) {
      task.lastRunMs = nowMs;
      task.callback();
    }
  }
}

void Scheduler::setEnabled(const char* name, bool enabled) {
  if (!name) {
    return;
  }

  for (uint8_t i = 0; i < taskCount; i++) {
    if (tasks[i].name && strcmp(tasks[i].name, name) == 0) {
      tasks[i].enabled = enabled;
      return;
    }
  }
}

void Scheduler::reset() {
  taskCount = 0;
}

}  // namespace InfoTermScheduler
