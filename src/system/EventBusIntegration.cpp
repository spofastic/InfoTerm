#include "EventBusIntegration.h"
#include "EventBus.h"

namespace InfoTermEventBusIntegration {

bool begin() {
  return true;
}

static bool publish(InfoTermEventBus::EventType type, const char* source, const char* key=nullptr) {
  InfoTermEventBus::RuntimeEventBus.publish(type, source, key, nullptr);
  return true;
}

bool publishConfigurationChanged(const char* source, const char* key) {
  return publish(InfoTermEventBus::EventType::ConfigChanged, source, key);
}

bool publishWidgetChanged(const char* source, const char* key) {
  return publish(InfoTermEventBus::EventType::WidgetChanged, source, key);
}

bool publishWeatherUpdated(const char* source, const char* key) {
  return publish(InfoTermEventBus::EventType::WeatherUpdated, source, key);
}

bool publishMQTTConnected(const char* source) {
  return publish(InfoTermEventBus::EventType::MQTTConnected, source);
}

bool publishMQTTDisconnected(const char* source) {
  return publish(InfoTermEventBus::EventType::MQTTDisconnected, source);
}

bool publishDisplayChanged(const char* source, const char* key) {
  return publish(InfoTermEventBus::EventType::DisplayChanged, source, key);
}

bool publishWiFiConnected(const char* source) {
  return publish(InfoTermEventBus::EventType::WiFiConnected, source);
}

bool publishWiFiDisconnected(const char* source) {
  return publish(InfoTermEventBus::EventType::WiFiDisconnected, source);
}

bool publishStorageChanged(const char* source, const char* key) {
  return publish(InfoTermEventBus::EventType::StorageChanged, source, key);
}

bool publishSystemTick(const char* source) {
  return publish(InfoTermEventBus::EventType::SystemTick, source);
}

}  // namespace InfoTermEventBusIntegration
