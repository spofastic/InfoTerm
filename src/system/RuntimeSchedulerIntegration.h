#pragma once

#include <Arduino.h>

// 0.9.5 Scheduler Integration.
// Central runtime bridge for migrating periodic millis()-based work
// into the scheduler without changing all modules at once.
namespace InfoTermRuntimeSchedulerIntegration {

using TaskCallback = void (*)();

enum class RuntimeTask : uint8_t {
  WiFiReconnect,
  MQTTLoop,
  MQTTReconnect,
  WeatherUpdate,
  DisplayRefresh,
  WidgetRefresh,
  DataPointSync,
  SystemStatus,
  WebServerHandle,
  WifiScan,
  RssUpdate,
  RssScroll
};

const char* taskName(RuntimeTask task);
uint32_t defaultIntervalMs(RuntimeTask task);

bool begin();
bool registerTask(RuntimeTask task, TaskCallback callback, uint32_t intervalMs = 0);
void tick();
bool isInitialized();

}
