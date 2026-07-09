#include "EventBus.h"

namespace InfoTermEventBus {

EventBus RuntimeEventBus;

EventBus::EventBus() : subscriptionCount(0) {}

bool EventBus::subscribe(EventType type, EventHandler handler, bool enabled) {
  if (!handler || subscriptionCount >= MAX_SUBSCRIPTIONS) {
    return false;
  }

  subscriptions[subscriptionCount] = {type, handler, enabled};
  subscriptionCount++;
  return true;
}

void EventBus::publish(EventType type, const char* source, const char* key, const char* value) {
  Event event{type, source, key, value, millis()};
  publish(event);
}

void EventBus::publish(const Event& event) {
  for (uint8_t i = 0; i < subscriptionCount; i++) {
    const Subscription& sub = subscriptions[i];
    if (sub.enabled && sub.handler && sub.type == event.type) {
      sub.handler(event);
    }
  }
}

void EventBus::reset() {
  subscriptionCount = 0;
}

const char* eventTypeName(EventType type) {
  switch (type) {
    case EventType::ConfigChanged: return "ConfigChanged";
    case EventType::WidgetChanged: return "WidgetChanged";
    case EventType::MQTTConnected: return "MQTTConnected";
    case EventType::MQTTDisconnected: return "MQTTDisconnected";
    case EventType::WeatherUpdated: return "WeatherUpdated";
    case EventType::DisplayChanged: return "DisplayChanged";
    case EventType::WiFiConnected: return "WiFiConnected";
    case EventType::WiFiDisconnected: return "WiFiDisconnected";
    case EventType::StorageChanged: return "StorageChanged";
    case EventType::SystemTick: return "SystemTick";
    default: return "Unknown";
  }
}

}  // namespace InfoTermEventBus
