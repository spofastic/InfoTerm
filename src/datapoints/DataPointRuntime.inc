/******************************************************************************
 * InfoTerm
 * Module  : DataPoint runtime
 * Version : 0.6.3
 *
 * Runtime DataPoint helpers migrated from InfoTerm.ino.
 * This keeps the existing global DataPointManager and behavior unchanged while
 * moving registration, update and explorer formatting helpers into the
 * DataPoint module area.
 ******************************************************************************/

static String dataPointSourceText(DataPointSource source) {
  switch (source) {
    case DATA_SOURCE_WEATHER: return "Weather";
    case DATA_SOURCE_MQTT: return "MQTT";
    case DATA_SOURCE_SYSTEM: return "System";
    case DATA_SOURCE_INTERNAL: return "Internal";
    default: return "Unknown";
  }
}

static String dataPointAgeText(const DataPoint& point) {
  if (!point.valid || point.lastUpdateMs == 0) return "--";
  unsigned long ageMs = millis() - point.lastUpdateMs;
  unsigned long seconds = ageMs / 1000UL;
  if (seconds < 60UL) return String(seconds) + " s";
  unsigned long minutes = seconds / 60UL;
  if (minutes < 60UL) return String(minutes) + " min";
  unsigned long hours = minutes / 60UL;
  return String(hours) + " h";
}

static bool weatherJsonFloat(JsonVariant root, const char* key, float& outValue) {
  if (root.isNull() || !root.is<JsonObject>()) return false;

  JsonVariant variant = root[key];
  if (!variant.isNull()) {
    outValue = variant.as<float>();
    return true;
  }

  const char* containers[] = {"observation", "obs", "data", "state", "attributes", "values"};
  for (const char* container : containers) {
    JsonVariant nested = root[container];
    if (nested.isNull() || !nested.is<JsonObject>()) continue;
    variant = nested[key];
    if (!variant.isNull()) {
      outValue = variant.as<float>();
      return true;
    }
  }

  return false;
}

struct WeatherFlowObservationField {
  const char* key;
  const char* label;
  const char* unit;
  uint8_t decimals;
};

static const WeatherFlowObservationField WEATHERFLOW_OBSERVATION_FIELDS[] = {
  {"absolute_humidity_gm3", "Absolute Luftfeuchte", "g/m3", 1},
  {"air_density_kgm3", "Luftdichte", "kg/m3", 3},
  {"delta_t_c", "Delta-T", "C", 1},
  {"dewpoint_c", "Taupunkt", "C", 1},
  {"humidity_percent", "Relative Luftfeuchte", "%", 0},
  {"illuminance_lux", "Beleuchtungsstaerke", "lx", 0},
  {"lightning_count", "Blitze", "", 0},
  {"lightning_count_1h", "Blitze 1 h", "", 0},
  {"lightning_count_3h", "Blitze 3 h", "", 0},
  {"lightning_count_today", "Blitze heute", "", 0},
  {"lightning_distance_km", "Blitzentfernung", "km", 1},
  {"lightning_energy", "Blitzenergie", "", 0},
  {"rain_duration_today_min", "Regendauer heute", "min", 0},
  {"rain_duration_yesterday_min", "Regendauer gestern", "min", 0},
  {"rain_rate_mm_h", "Regenrate", "mm/h", 2},
  {"rain_today_mm", "Regen heute", "mm", 2},
  {"rain_yesterday_mm", "Regen gestern", "mm", 2},
  {"sea_level_pressure_hpa", "Luftdruck Meeresspiegel", "hPa", 1},
  {"solar_radiation_wm2", "Solarstrahlung", "W/m2", 0},
  {"station_pressure_hpa", "Stationsluftdruck", "hPa", 1},
  {"temperature_c", "Temperatur", "C", 1},
  {"uv_index", "UV-Index", "", 1},
  {"wbgt_c", "WBGT", "C", 1},
  {"wetbulb_c", "Feuchtkugeltemperatur", "C", 1},
  {"wind_bearing_avg_deg", "Windrichtung Mittel", "deg", 0},
  {"wind_gust_ms", "Windboee", "m/s", 1},
  {"wind_lull_ms", "Windminimum", "m/s", 1},
  {"wind_speed_avg_ms", "Windgeschwindigkeit Mittel", "m/s", 1},
};

static String weatherFlowDataPointId(const char* key) {
  return String("weatherflow.") + String(key);
}

static String formatWeatherFlowObservationValue(float value, uint8_t decimals) {
  return String(value, static_cast<unsigned int>(decimals));
}

static bool weatherFlowObservationFloat(JsonVariant root, const char* key, float& outValue) {
  if (weatherJsonFloat(root, key, outValue)) return true;

  if (strcmp(key, "temperature_c") == 0) return weatherJsonFloat(root, "air_temperature", outValue);
  if (strcmp(key, "humidity_percent") == 0) return weatherJsonFloat(root, "relative_humidity", outValue) || weatherJsonFloat(root, "humidity", outValue) || weatherJsonFloat(root, "humidity_pct", outValue);
  if (strcmp(key, "station_pressure_hpa") == 0) return weatherJsonFloat(root, "station_pressure", outValue) || weatherJsonFloat(root, "pressure", outValue) || weatherJsonFloat(root, "pressure_hpa", outValue);
  if (strcmp(key, "sea_level_pressure_hpa") == 0) return weatherJsonFloat(root, "sealevel_pressure", outValue);
  if (strcmp(key, "wind_speed_avg_ms") == 0) return weatherJsonFloat(root, "wind_speed_avg", outValue) || weatherJsonFloat(root, "wind_speed", outValue) || weatherJsonFloat(root, "wind_speed_ms", outValue);
  if (strcmp(key, "wind_bearing_avg_deg") == 0) return weatherJsonFloat(root, "wind_direction_avg", outValue) || weatherJsonFloat(root, "wind_direction", outValue) || weatherJsonFloat(root, "wind_direction_deg", outValue);
  if (strcmp(key, "rain_today_mm") == 0) return weatherJsonFloat(root, "rain_today", outValue) || weatherJsonFloat(root, "rain", outValue) || weatherJsonFloat(root, "precipitation", outValue) || weatherJsonFloat(root, "precipitation_mm", outValue);
  if (strcmp(key, "rain_rate_mm_h") == 0) return weatherJsonFloat(root, "rain_rate", outValue);
  if (strcmp(key, "uv_index") == 0) return weatherJsonFloat(root, "uv", outValue);

  return false;
}

static bool syncWeatherFlowObservationDataPoints(JsonDocument& doc) {
  bool updated = false;
  JsonVariant root = doc.as<JsonVariant>();
  for (const WeatherFlowObservationField& field : WEATHERFLOW_OBSERVATION_FIELDS) {
    float value = NAN;
    if (!weatherFlowObservationFloat(root, field.key, value)) continue;
    dataPointManager.setValue(
      weatherFlowDataPointId(field.key),
      formatWeatherFlowObservationValue(value, field.decimals),
      field.unit,
      true
    );
    updated = true;
  }
  return updated;
}

void registerBuiltInDataPoints() {
  dataPointManager.clear();

  dataPointManager.registerPoint("weather.temperature", TXT(L_WEATHER_OUTDOOR), "C", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.rain", TXT(L_WEATHER_RAIN), "mm", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.wind", TXT(L_WEATHER_WIND), "m/s", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.uv", TXT(L_WEATHER_UV), "", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.kp", TXT(L_WEATHER_KP), "", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.sunrise", TXT(L_SUNRISE), "", DATA_SOURCE_WEATHER);
  dataPointManager.registerPoint("weather.sunset", TXT(L_SUNSET), "", DATA_SOURCE_WEATHER);

  for (const WeatherFlowObservationField& field : WEATHERFLOW_OBSERVATION_FIELDS) {
    dataPointManager.registerPoint(weatherFlowDataPointId(field.key), field.label, field.unit, DATA_SOURCE_WEATHER);
  }

  dataPointManager.registerPoint("system.wifi", TXT(L_STATUS_WIFI), "", DATA_SOURCE_SYSTEM);
  dataPointManager.registerPoint("system.ip", TXT(L_STATUS_IP), "", DATA_SOURCE_SYSTEM);
  dataPointManager.registerPoint("system.uptime", TXT(L_STATUS_UPTIME), "", DATA_SOURCE_SYSTEM);
  dataPointManager.registerPoint("system.version", "Firmware", "", DATA_SOURCE_SYSTEM);
  dataPointManager.registerPoint("system.freeHeap", TXT(L_STATUS_FREEHEAP), "KB", DATA_SOURCE_SYSTEM);

  for (int i = 0; i < mqttValueCount; i++) {
    String id = "mqtt." + String(i + 1);
    dataPointManager.registerPoint(id, mqttLabels[i], mqttUnits[i], DATA_SOURCE_MQTT);
  }
}

void syncDataPointsFromRuntime() {
  dataPointManager.setValue("weather.temperature", tempText(), unitKey == "imperial" ? "F" : "C", !isnan(tempC));
  dataPointManager.setValue("weather.rain", rainText(), unitKey == "imperial" ? "in" : "mm", !isnan(precipMm));
  dataPointManager.setValue("weather.wind", windText(), unitKey == "imperial" ? "mph" : "m/s", !isnan(windSpeedMs));
  dataPointManager.setValue("weather.uv", uvText(), "", !isnan(uvIndex));
  dataPointManager.setValue("weather.kp", kpText(), "", !isnan(kpIndex));
  dataPointManager.setValue("weather.sunrise", sunriseMin >= 0 ? formatMinuteOfDay(sunriseMin) : "--:--", "", sunriseMin >= 0);
  dataPointManager.setValue("weather.sunset", sunsetMin >= 0 ? formatMinuteOfDay(sunsetMin) : "--:--", "", sunsetMin >= 0);

  dataPointManager.setValue("system.wifi", wifiStatusText(), "", true);
  dataPointManager.setValue("system.ip", ipText(), "", WiFi.status() == WL_CONNECTED);
  dataPointManager.setValue("system.uptime", uptimeText(), "", true);
  dataPointManager.setValue("system.version", String(INFOTERM_APP_VERSION), "", true);
  dataPointManager.setValue("system.freeHeap", String(ESP.getFreeHeap() / 1024), "KB", true);

  for (int i = 0; i < mqttValueCount; i++) {
    String id = "mqtt." + String(i + 1);
    dataPointManager.setLabel(id, mqttLabels[i]);
    dataPointManager.setUnit(id, mqttUnits[i]);
    dataPointManager.setValue(id, mqttFormattedValue(i), mqttUnits[i], mqttValueEnabled[i] && mqttValues[i] != "--");
  }
}
