#include "RuntimeSchedulerIntegration.h"
#include "Logger.h"
#include "Scheduler.h"

namespace InfoTermRuntimeSchedulerIntegration {

static bool initialized = false;

const char* taskName(RuntimeTask task) {
  switch (task) {
    case RuntimeTask::WiFiReconnect: return "wifi.reconnect";
    case RuntimeTask::MQTTLoop: return "mqtt.loop";
    case RuntimeTask::MQTTReconnect: return "mqtt.reconnect";
    case RuntimeTask::WeatherUpdate: return "weather.update";
    case RuntimeTask::DisplayRefresh: return "display.refresh";
    case RuntimeTask::WidgetRefresh: return "widgets.refresh";
    case RuntimeTask::DataPointSync: return "datapoints.sync";
    case RuntimeTask::SystemStatus: return "system.status";
    case RuntimeTask::WebServerHandle: return "webserver.handle";
    case RuntimeTask::WifiScan: return "wifi.scan";
    case RuntimeTask::RssUpdate: return "rss.update";
    case RuntimeTask::RssScroll: return "rss.scroll";
    default: return "unknown";
  }
}

uint32_t defaultIntervalMs(RuntimeTask task) {
  switch (task) {
    case RuntimeTask::WebServerHandle: return 20;
    case RuntimeTask::MQTTLoop: return 50;
    case RuntimeTask::DisplayRefresh: return 250;
    case RuntimeTask::WidgetRefresh: return 1000;
    case RuntimeTask::DataPointSync: return 1000;
    case RuntimeTask::SystemStatus: return 5000;
    case RuntimeTask::WiFiReconnect: return 5000;
    case RuntimeTask::MQTTReconnect: return 5000;
    case RuntimeTask::WeatherUpdate: return 60000;
    case RuntimeTask::WifiScan: return 1000;
    case RuntimeTask::RssUpdate: return 60000;
    case RuntimeTask::RssScroll: return 100;
    default: return 1000;
  }
}

bool begin() {
  initialized = true;
  Logger::info("[RuntimeScheduler] integration initialized");
  return true;
}

bool registerTask(RuntimeTask task, TaskCallback callback, uint32_t intervalMs) {
  if (!initialized) {
    begin();
  }

  const uint32_t interval = intervalMs > 0 ? intervalMs : defaultIntervalMs(task);
  return InfoTermScheduler::RuntimeScheduler.add(taskName(task), interval, callback, true);
}

void tick() {
  if (!initialized) {
    begin();
  }
  InfoTermScheduler::RuntimeScheduler.tick();
}

bool isInitialized() {
  return initialized;
}

}
