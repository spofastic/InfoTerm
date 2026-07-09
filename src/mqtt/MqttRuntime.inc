/******************************************************************************
 * InfoTerm
 * Module  : MQTT runtime
 * Version : 0.6.1
 *
 * Contains the legacy MQTT runtime implementation extracted from InfoTerm.ino.
 * The code is included into the main compilation unit to keep the existing
 * global runtime state and callbacks unchanged while reducing the size of the
 * main firmware file.
 ******************************************************************************/

#include "system/EventBusIntegration.h"

// =========================================================
// MQTT / HOME ASSISTANT
// =========================================================
static bool parseNumericPayload(const byte* payload, unsigned int length, float& outValue) {
  char buf[40];
  unsigned int copyLen = min(length, (unsigned int)(sizeof(buf) - 1));
  memcpy(buf, payload, copyLen);
  buf[copyLen] = '\0';

  String s = String(buf);
  s.trim();
  s.replace(",", ".");
  if (s.length() == 0) return false;

  char* endPtr = nullptr;
  float v = strtof(s.c_str(), &endPtr);
  if (endPtr == s.c_str()) return false;

  outValue = v;
  return true;
}

static void markMqttWeatherUpdated() {
  lastMqttWeatherFetch = time(nullptr);
  lastWeatherFetch = lastMqttWeatherFetch;
  lastSyncTime = lastMqttWeatherFetch;
  // Repaint weather widgets in place, without a full-screen redraw (fixed
  // 1.0.1): the old pageDirty here forced drawCurrentPageFull() with a
  // fillScreen on every incoming WeatherFlow observation - the visible
  // top-to-bottom flicker every few seconds. Dropping pageDirty alone left
  // the weather widgets with no repaint trigger (they stopped updating), so
  // mirror the MQTT DataPoint path (see mqttCallback): invalidate the widget
  // caches so the next 1 Hz updateCurrentPageDynamic() redraws the affected
  // widgets via sprite push (no fillScreen, no flicker).
  clearHomeSlotCaches();
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) cacheCustomTabSlots[t][i] = "";
  }
  dataDirty = true;
  InfoTermEventBusIntegration::publishWeatherUpdated("mqtt", "weather");
}

static void applyWeatherJsonPayload(const byte* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) return;

  bool changed = false;
  JsonVariant root = doc.as<JsonVariant>();
  // Generic/old JSON keys
  changed |= weatherJsonFloat(root, "temperature", tempC);
  changed |= weatherJsonFloat(root, "temp", tempC);
  changed |= weatherJsonFloat(root, "temperature_c", tempC);
  changed |= weatherJsonFloat(root, "temp_min", tempMinC);
  changed |= weatherJsonFloat(root, "temperature_min", tempMinC);
  changed |= weatherJsonFloat(root, "temp_max", tempMaxC);
  changed |= weatherJsonFloat(root, "temperature_max", tempMaxC);
  changed |= weatherJsonFloat(root, "rain", precipMm);
  changed |= weatherJsonFloat(root, "precipitation", precipMm);
  changed |= weatherJsonFloat(root, "precipitation_mm", precipMm);
  changed |= weatherJsonFloat(root, "wind_speed", windSpeedMs);
  changed |= weatherJsonFloat(root, "wind_speed_ms", windSpeedMs);
  changed |= weatherJsonFloat(root, "wind_direction", windDirectionDeg);
  changed |= weatherJsonFloat(root, "wind_direction_deg", windDirectionDeg);
  changed |= weatherJsonFloat(root, "uv", uvIndex);
  changed |= weatherJsonFloat(root, "uv_index", uvIndex);
  changed |= weatherJsonFloat(root, "humidity", humidityPct);
  changed |= weatherJsonFloat(root, "humidity_pct", humidityPct);
  changed |= weatherJsonFloat(root, "humidity_percent", humidityPct);
  changed |= weatherJsonFloat(root, "pressure", pressureHPa);
  changed |= weatherJsonFloat(root, "pressure_hpa", pressureHPa);

  // WetterFlow2MQTT / Home Assistant observation JSON keys
  changed |= weatherJsonFloat(root, "air_temperature", tempC);
  changed |= weatherJsonFloat(root, "relative_humidity", humidityPct);
  changed |= weatherJsonFloat(root, "station_pressure", pressureHPa);
  changed |= weatherJsonFloat(root, "station_pressure_hpa", pressureHPa);
  changed |= weatherJsonFloat(root, "sealevel_pressure", pressureHPa);
  changed |= weatherJsonFloat(root, "sea_level_pressure_hpa", pressureHPa);
  changed |= weatherJsonFloat(root, "wind_speed_avg", windSpeedMs);
  changed |= weatherJsonFloat(root, "wind_speed_avg_ms", windSpeedMs);
  changed |= weatherJsonFloat(root, "wind_direction_avg", windDirectionDeg);
  changed |= weatherJsonFloat(root, "wind_bearing_avg_deg", windDirectionDeg);

  // Prefer rain_today as daily precipitation display. If only rain_rate exists, use that.
  if (weatherJsonFloat(root, "rain_today_mm", precipMm) || weatherJsonFloat(root, "rain_today", precipMm)) {
    changed = true;
  } else if (weatherJsonFloat(root, "rain_rate_mm_h", precipMm) || weatherJsonFloat(root, "rain_rate", precipMm)) {
    changed = true;
  }

  bool weatherFlowDataPointsChanged = syncWeatherFlowObservationDataPoints(doc);
  if (weatherFlowDataPointsChanged) {
    changed = true;
  }

  if (changed) {
    markMqttWeatherUpdated();
    syncDataPointsFromRuntime();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);

  String displayValue = "";
  for (unsigned int i = 0; i < length && i < 48; i++) {
    char c = (char)payload[i];
    if (c >= 32 && c <= 126) displayValue += c;
  }
  displayValue.trim();
  if (displayValue.length() == 0) displayValue = "--";
  for (int i = 0; i < mqttValueCount; i++) {
    if (mqttValueEnabled[i] && mqttTopics[i].length() > 0 && t == mqttTopics[i]) {
      mqttValues[i] = displayValue;
      String formattedValue = mqttFormattedValue(i);
      dataPointManager.setLabel("mqtt." + String(i + 1), mqttLabels[i]);
      dataPointManager.setUnit("mqtt." + String(i + 1), mqttUnits[i]);
      dataPointManager.setValue("mqtt." + String(i + 1), formattedValue, mqttUnits[i], true);
      if (currentPage == PAGE_MQTT) dataDirty = true;
      clearHomeSlotCaches();
    }
  }

  if (!mqttWeatherEnabled) return;

  if (t == mqttWeatherflowTopic || t == String(MQTT_TOPIC_WEATHER_JSON)) {
    applyWeatherJsonPayload(payload, length);
    return;
  }

  float value = NAN;
  if (!parseNumericPayload(payload, length, value)) return;

  if (t == MQTT_TOPIC_TEMP) {
    tempC = value;
  } else if (t == MQTT_TOPIC_TEMP_MIN) {
    tempMinC = value;
  } else if (t == MQTT_TOPIC_TEMP_MAX) {
    tempMaxC = value;
  } else if (t == MQTT_TOPIC_RAIN) {
    precipMm = value;
  } else if (t == MQTT_TOPIC_WIND_SPEED) {
    windSpeedMs = value;
  } else if (t == MQTT_TOPIC_WIND_DIR) {
    windDirectionDeg = value;
  } else if (t == MQTT_TOPIC_UV) {
    uvIndex = value;
  } else if (t == MQTT_TOPIC_HUMIDITY) {
    humidityPct = value;
  } else if (t == MQTT_TOPIC_PRESSURE) {
    pressureHPa = value;
  } else {
    return;
  }

  markMqttWeatherUpdated();
}

static void mqttSubscribeWeatherTopics() {
  mqttClient.subscribe(mqttWeatherflowTopic.c_str());
  mqttClient.subscribe(MQTT_TOPIC_WEATHER_JSON);
  mqttClient.subscribe(MQTT_TOPIC_TEMP);
  mqttClient.subscribe(MQTT_TOPIC_TEMP_MIN);
  mqttClient.subscribe(MQTT_TOPIC_TEMP_MAX);
  mqttClient.subscribe(MQTT_TOPIC_RAIN);
  mqttClient.subscribe(MQTT_TOPIC_WIND_SPEED);
  mqttClient.subscribe(MQTT_TOPIC_WIND_DIR);
  mqttClient.subscribe(MQTT_TOPIC_UV);
  mqttClient.subscribe(MQTT_TOPIC_HUMIDITY);
  mqttClient.subscribe(MQTT_TOPIC_PRESSURE);
  for (int i = 0; i < mqttValueCount; i++) {
    if (mqttValueEnabled[i] && mqttTopics[i].length() > 0) {
      mqttClient.subscribe(mqttTopics[i].c_str());
    }
  }
}

void ensureMQTTVerbunden();

static bool mqttDisplayHasEnabledValues();

void setupMQTT() {
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(4096);
}

static String mqttStateText() {
  if (!mqttWeatherEnabled && !mqttDisplayHasEnabledValues()) return String(TXT(L_STATUS_MQTT_OFF));
  return mqttClient.connected() ? String(TXT(L_STATUS_ONLINE)) : String(TXT(L_STATUS_OFFLINE));
}

static bool mqttWeatherIsFresh() {
  if (!mqttWeatherEnabled || lastMqttWeatherFetch <= 0) return false;
  time_t now = time(nullptr);
  if (now < 1700000000) return true;  // NTP not ready yet: trust the latest retained MQTT payload.
  return (now - lastMqttWeatherFetch) < (30 * 60);
}

void reloadMQTTConfig() {
  if (mqttClient.connected()) {
    mqttClient.disconnect();
    InfoTermEventBusIntegration::publishMQTTDisconnected("mqtt");
  }
  mqttWasVerbunden = false;
  setupMQTT();
  lastMqttReconnectAttemptMs = 0;
  ensureMQTTVerbunden();
}

static bool mqttDisplayHasEnabledValues() {
  for (int i = 0; i < mqttValueCount; i++) {
    if (mqttValueEnabled[i] && mqttTopics[i].length() > 0) return true;
  }
  return false;
}

void ensureMQTTVerbunden() {
  if ((!mqttWeatherEnabled && !mqttDisplayHasEnabledValues()) || !wifiEnabled || WiFi.status() != WL_CONNECTED) {
    if (mqttWasVerbunden) {
      mqttWasVerbunden = false;
      InfoTermEventBusIntegration::publishMQTTDisconnected("mqtt");
    }
    return;
  }
  if (mqttClient.connected()) {
    mqttWasVerbunden = true;
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttReconnectAttemptMs < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttReconnectAttemptMs = now;

  bool ok = false;
  if (mqttUser.length() > 0) {
    ok = mqttClient.connect(MQTT_CLIENT_ID, mqttUser.c_str(), mqttPass.c_str());
  } else {
    ok = mqttClient.connect(MQTT_CLIENT_ID);
  }

  if (ok) {
    mqttWasVerbunden = true;
    mqttSubscribeWeatherTopics();
    // Only dataDirty (fixed 1.0.1): the MQTT status change is shown by the
    // 1 Hz updateStatusDynamic(); a full redraw here flickered the screen on
    // every MQTT (re)connect.
    dataDirty = true;
    InfoTermEventBusIntegration::publishMQTTConnected("mqtt");
  } else {
    if (mqttWasVerbunden) {
      InfoTermEventBusIntegration::publishMQTTDisconnected("mqtt");
    }
    mqttWasVerbunden = false;
  }
}
