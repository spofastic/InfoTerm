#pragma once

#include <Arduino.h>

namespace InfoTermEventBus {

enum class EventType : uint8_t {
  ConfigChanged,
  WidgetChanged,
  MQTTConnected,
  MQTTDisconnected,
  WeatherUpdated,
  DisplayChanged,
  WiFiConnected,
  WiFiDisconnected,
  StorageChanged,
  SystemTick
};

struct Event {
  EventType type;
  const char* source;
  const char* key;
  const char* value;
  uint32_t timestampMs;
};

using EventHandler = void (*)(const Event& event);

struct Subscription {
  EventType type;
  EventHandler handler;
  bool enabled;
};

class EventBus {
 public:
  EventBus();

  bool subscribe(EventType type, EventHandler handler, bool enabled = true);
  void publish(EventType type, const char* source = nullptr, const char* key = nullptr, const char* value = nullptr);
  void publish(const Event& event);
  void reset();

 private:
  static const uint8_t MAX_SUBSCRIPTIONS = 24;
  Subscription subscriptions[MAX_SUBSCRIPTIONS];
  uint8_t subscriptionCount;
};

extern EventBus RuntimeEventBus;

const char* eventTypeName(EventType type);

}  // namespace InfoTermEventBus
