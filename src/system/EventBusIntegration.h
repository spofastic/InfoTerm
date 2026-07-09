#pragma once
namespace InfoTermEventBusIntegration{
bool begin();
bool publishConfigurationChanged(const char* source=nullptr, const char* key=nullptr);
bool publishWidgetChanged(const char* source=nullptr, const char* key=nullptr);
bool publishWeatherUpdated(const char* source=nullptr, const char* key=nullptr);
bool publishMQTTConnected(const char* source=nullptr);
bool publishMQTTDisconnected(const char* source=nullptr);
bool publishDisplayChanged(const char* source=nullptr, const char* key=nullptr);
bool publishWiFiConnected(const char* source=nullptr);
bool publishWiFiDisconnected(const char* source=nullptr);
bool publishStorageChanged(const char* source=nullptr, const char* key=nullptr);
bool publishSystemTick(const char* source=nullptr);
}
