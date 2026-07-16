// InfoTerm configuration backup/export runtime
//
// Serializes the full runtime configuration to JSON for JsonManager and
// BackupRestoreManager. Included directly into the InfoTerm.cpp unity build
// so it has access
// to the same global runtime state as the NVS-based persistence in
// StoragePersistRuntime.inc.
//
// This file includes <LittleFS.h> and mounts LittleFS itself, since this is
// the only place in the project that needs it (the WebGUI itself is fully
// inline HTML/CSS/JS, see docs/ARCHITECTURE.md).

#include <LittleFS.h>
#include "config/ConfigSchema.h"

// Central LittleFS mount for backup/restore and the WebGUI upload handlers.
// Formatting can never help the import/upload paths - it would destroy the
// file that was just uploaded - so it is only allowed, and logged, where a
// fresh (never formatted) partition must be bootstrapped: the export path.
// The partition only ever carries transient export/import/upload files, so
// that one-time format cannot lose persistent data.
static bool mountConfigFilesystem(bool allowFormatOnFail) {
  if (LittleFS.begin(false)) return true;
  if (!allowFormatOnFail) {
    Logger::error("LittleFS mount failed (auto-format disabled on this path)");
    return false;
  }
  Logger::warning("LittleFS mount failed - formatting config filesystem");
  return LittleFS.begin(true);
}

bool serializeConfigurationToJson(String& jsonOut) {
  JsonDocument doc;
  doc["schemaVersion"] = InfoTermConfigSchema::CURRENT;
  doc["firmwareVersion"] = INFOTERM_APP_VERSION;

  JsonObject general = doc["general"].to<JsonObject>();
  general["accent"] = accentColorKey;
  general["border"] = borderColorKey;
  general["bg"] = backgroundColorKey;
  general["text"] = textColorKey;
  general["menuBg"] = menuBgColorKey;
  general["buttonBg"] = buttonBgColorKey;
  general["widgetBg"] = widgetBgColorKey;
  general["widgetBorder"] = widgetBordersEnabled;
  general["widgetFont"] = widgetFontKey;
  general["units"] = unitKey;
  general["region"] = regionFormatKey;
  general["lang"] = languageCode();
  general["tz"] = timezoneKey;
  general["deviceName"] = deviceName;
  general["locationName"] = locationName;
  general["lat"] = LAT;
  general["lng"] = LNG;
  general["sleepMin"] = sleepIntervalMin;
  general["logLevel"] = logLevelValue;
  general["flashMode"] = flashModeEnabled;
  general["wifiEnabled"] = wifiEnabled;
  general["wifiActiveIdx"] = wifiActiveIndex;
  // Wi-Fi slot SSIDs are included; passwords are deliberately NEVER exported
  // (write-only, like the MQTT/WebGUI passwords) so a backup file carries no
  // Wi-Fi secrets.
  JsonArray wifiSsids = general["wifiSsids"].to<JsonArray>();
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) wifiSsids.add(wifiSlotSsid[i]);
  general["ipMode"] = ipMode;
  general["ipStatic"] = staticIp;
  general["ipGw"] = staticGateway;
  general["ipMask"] = staticSubnet;
  general["ipDns"] = staticDns;
  // Secrets are deliberately NOT exported (see docs/CONFIGURATION.md): the
  // backup file is downloaded over the network and would otherwise hand out
  // the WireGuard private key and MQTT password in clear text. After a
  // restore, re-upload the WireGuard .conf and re-enter the MQTT password.
  // (vpnPrivateKey, vpnPeerPublicKey, mqttPass omitted on purpose.)
  general["vpnHost"] = vpnEndpointHost;
  general["vpnPort"] = vpnEndpointPort;
  general["vpnLocalIp"] = vpnLocalAddress;
  general["vpnDns"] = vpnDns;
  general["vpnAllowedIps"] = vpnAllowedIps;
  general["vpnName"] = vpnName;
  general["nightMode"] = nightModeEnabled;
  general["nightForce"] = nightModeForced;
  general["nightBrightPct"] = nightBrightnessPct;
  general["rotate180"] = displayRotated180;
  general["tabCycle"] = tabCycleEnabled;
  general["tabCycleSec"] = tabCycleIntervalSec;
  general["mqttOn"] = mqttWeatherEnabled;
  general["mqttFallback"] = mqttFallbackOpenMeteo;
  general["mqttHost"] = mqttHost;
  general["mqttPort"] = mqttPort;
  general["mqttUser"] = mqttUser;
  general["mqttTopic"] = mqttWeatherflowTopic;
  // WebGUI login (1.0.7): the username is exported like mqttUser; the
  // password is a secret and deliberately NOT exported (see note above).
  general["webUser"] = webUser;

  doc["mqttValueCount"] = mqttValueCount;
  JsonArray mqttValues = doc["mqttValues"].to<JsonArray>();
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    JsonObject v = mqttValues.add<JsonObject>();
    v["label"] = mqttLabels[i];
    v["topic"] = mqttTopics[i];
    v["unit"] = mqttUnits[i];
    v["decimals"] = mqttDecimals[i];
    v["enabled"] = mqttValueEnabled[i];
  }

  JsonArray homeSlots = doc["homeSlots"].to<JsonArray>();
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    homeSlots.add(widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]));
  }

  // RSS feed URLs (1.0.10, issue #7) - plain URLs, no secrets involved.
  JsonArray rssFeeds = doc["rssFeeds"].to<JsonArray>();
  for (int i = 0; i < rssFeedCount; i++) {
    rssFeeds.add(rssFeedUrls[i]);
  }

  JsonArray tabs = doc["tabs"].to<JsonArray>();
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    JsonObject tab = tabs.add<JsonObject>();
    tab["name"] = customTabNames[t];
    tab["enabled"] = customTabEnabled[t];
    tab["content"] = customTabContent[t];
    tab["feed"] = customTabFeedIndex[t];
    JsonArray slots = tab["slots"].to<JsonArray>();
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      slots.add(widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]));
    }
  }

  serializeJson(doc, jsonOut);
  return true;
}

bool serializeConfigurationToFile(const char* path) {
  String json;
  if (!serializeConfigurationToJson(json)) return false;
  if (!mountConfigFilesystem(true)) return false;

  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = file.print(json);
  file.close();
  return written == json.length();
}

// Applies a parsed configuration document to the runtime globals and
// persists it via the existing NVS-based persistence. Rejects an
// incompatible/unknown schema version outright, leaving the current runtime
// configuration untouched. Shared by the String- and the streaming
// File-based entry points below.
static bool applyConfigurationFromDoc(JsonDocument& doc) {
  uint16_t storedSchema = doc["schemaVersion"] | 0;
  if (storedSchema == 0 || storedSchema > InfoTermConfigSchema::CURRENT) return false;

  JsonObject general = doc["general"];
  if (general.isNull()) return false;

  accentColorKey = general["accent"] | accentColorKey;
  borderColorKey = general["border"] | borderColorKey;
  backgroundColorKey = general["bg"] | backgroundColorKey;
  textColorKey = general["text"] | textColorKey;
  menuBgColorKey = general["menuBg"] | menuBgColorKey;
  buttonBgColorKey = general["buttonBg"] | buttonBgColorKey;
  widgetBgColorKey = general["widgetBg"] | widgetBgColorKey;
  widgetBordersEnabled = general["widgetBorder"] | widgetBordersEnabled;
  widgetFontKey = general["widgetFont"] | widgetFontKey;
  unitKey = general["units"] | unitKey;
  regionFormatKey = general["region"] | regionFormatKey;
  uiLanguage = languageFromCode(general["lang"] | String(languageCode()));
  timezoneKey = sanitizeTimezoneKey(general["tz"] | timezoneKey);
  deviceName = general["deviceName"] | deviceName;
  locationName = general["locationName"] | locationName;
  LAT = general["lat"] | LAT;
  LNG = general["lng"] | LNG;
  sleepIntervalMin = general["sleepMin"] | sleepIntervalMin;
  logLevelValue = constrain((int)(general["logLevel"] | logLevelValue), 0, 3);
  Logger::setLevel(logLevelValue);
  flashModeEnabled = general["flashMode"] | flashModeEnabled;
  wifiEnabled = general["wifiEnabled"] | wifiEnabled;
  wifiActiveIndex = general["wifiActiveIdx"] | wifiActiveIndex;
  // Restore Wi-Fi slot SSIDs if present (passwords are never in a backup and
  // keep their current runtime/seed value).
  if (general["wifiSsids"].is<JsonArray>()) {
    JsonArray wifiSsids = general["wifiSsids"].as<JsonArray>();
    int i = 0;
    for (JsonVariant v : wifiSsids) {
      if (i >= WIFI_SLOT_COUNT) break;
      wifiSlotSsid[i++] = v.as<String>();
    }
  }
  ipMode = general["ipMode"] | ipMode;
  staticIp = general["ipStatic"] | staticIp;
  staticGateway = general["ipGw"] | staticGateway;
  staticSubnet = general["ipMask"] | staticSubnet;
  staticDns = general["ipDns"] | staticDns;
  // Secrets are no longer part of exports; only overwrite the current value
  // if a (hand-crafted) backup actually carries a non-empty one, so a normal
  // restore never wipes the existing key/password with an empty string.
  {
    const char* v = general["vpnPrivateKey"];
    if (v && *v) vpnPrivateKey = v;
    v = general["vpnPeerPublicKey"];
    if (v && *v) vpnPeerPublicKey = v;
  }
  vpnEndpointHost = general["vpnHost"] | vpnEndpointHost;
  vpnEndpointPort = (uint16_t)(general["vpnPort"] | vpnEndpointPort);
  vpnLocalAddress = general["vpnLocalIp"] | vpnLocalAddress;
  vpnDns = general["vpnDns"] | vpnDns;
  vpnAllowedIps = general["vpnAllowedIps"] | vpnAllowedIps;
  vpnName = general["vpnName"] | vpnName;
  nightModeEnabled = general["nightMode"] | nightModeEnabled;
  nightModeForced = general["nightForce"] | nightModeForced;
  nightBrightnessPct = constrain((int)(general["nightBrightPct"] | nightBrightnessPct), 10, 100);
  displayRotated180 = general["rotate180"] | displayRotated180;
  tabCycleEnabled = general["tabCycle"] | tabCycleEnabled;
  tabCycleIntervalSec = general["tabCycleSec"] | tabCycleIntervalSec;
  mqttWeatherEnabled = general["mqttOn"] | mqttWeatherEnabled;
  mqttFallbackOpenMeteo = general["mqttFallback"] | mqttFallbackOpenMeteo;
  mqttHost = general["mqttHost"] | mqttHost;
  mqttPort = (uint16_t)(general["mqttPort"] | mqttPort);
  mqttUser = general["mqttUser"] | mqttUser;
  {
    const char* v = general["mqttPass"];
    if (v && *v) mqttPass = v;  // see secret-restore note above
  }
  mqttWeatherflowTopic = general["mqttTopic"] | mqttWeatherflowTopic;
  {
    const char* v = general["webUser"];
    if (v && *v) webUser = v;  // never restore an empty username
    v = general["webPass"];
    if (v && *v) webPass = v;  // secret-restore note above: normal exports never carry it
  }

  mqttValueCount = constrain((int)(doc["mqttValueCount"] | mqttValueCount), 1, MQTT_VALUE_MAX);
  JsonArray mqttValues = doc["mqttValues"];
  if (!mqttValues.isNull()) {
    int i = 0;
    for (JsonObject v : mqttValues) {
      if (i >= MQTT_VALUE_COUNT) break;
      mqttLabels[i] = v["label"] | mqttLabels[i];
      mqttTopics[i] = v["topic"] | mqttTopics[i];
      mqttUnits[i] = v["unit"] | mqttUnits[i];
      mqttDecimals[i] = constrain((int)(v["decimals"] | mqttDecimals[i]), MQTT_DECIMALS_AUTO, 4);
      mqttValueEnabled[i] = v["enabled"] | mqttValueEnabled[i];
      i++;
    }
  }

  JsonArray homeSlots = doc["homeSlots"];
  if (!homeSlots.isNull()) {
    int i = 0;
    for (JsonVariant slot : homeSlots) {
      if (i >= HOME_SLOT_COUNT) break;
      applyWidgetSelectionKey(slot.as<String>(), homeWidgetSlots[i], homeWidgetDataPointIds[i]);
      i++;
    }
  }

  // RSS feeds (1.0.10): the array in the backup replaces the stored list
  // completely; fetch state is reset so all feeds are re-fetched.
  if (doc["rssFeeds"].is<JsonArray>()) {
    JsonArray rssFeeds = doc["rssFeeds"].as<JsonArray>();
    int i = 0;
    for (JsonVariant v : rssFeeds) {
      if (i >= RSS_FEED_COUNT) break;
      rssFeedUrls[i++] = v.as<String>();
    }
    rssFeedCount = i;
    for (int k = 0; k < RSS_FEED_COUNT; k++) {
      if (k >= rssFeedCount) rssFeedUrls[k] = "";
      rssResetFeedState(k);
    }
  }

  JsonArray tabs = doc["tabs"];
  if (!tabs.isNull()) {
    int t = 0;
    for (JsonObject tab : tabs) {
      if (t >= CUSTOM_TAB_COUNT) break;
      customTabNames[t] = tab["name"] | customTabNames[t];
      customTabEnabled[t] = tab["enabled"] | customTabEnabled[t];
      customTabContent[t] = tab["content"] | customTabContent[t];
      customTabFeedIndex[t] = tab["feed"] | customTabFeedIndex[t];
      JsonArray slots = tab["slots"];
      if (!slots.isNull()) {
        int i = 0;
        for (JsonVariant slot : slots) {
          if (i >= TAB_WIDGET_COUNT) break;
          applyWidgetSelectionKey(slot.as<String>(), customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]);
          i++;
        }
      }
      t++;
    }
  }

  sanitizeWidgetSlotsForMqttCount();
  sanitizeGeneralConfig();
  // A restored rotate180 must reach the panel, not only the flag - the
  // pageDirty repaint below draws into whatever rotation is active.
  applyDisplayRotation();
  persistRuntimeConfiguration();
  syncDataPointsFromRuntime();
  pageDirty = true;
  dataDirty = true;
  return true;
}

bool deserializeConfigurationFromJson(const String& json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  return applyConfigurationFromDoc(doc);
}

bool deserializeConfigurationFromFile(const char* path) {
  // Never auto-format here: a format on a transient mount failure would
  // destroy the just-uploaded import file (and the import fails anyway).
  if (!mountConfigFilesystem(false)) return false;
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  // Parse straight from the file handle instead of copying the whole file
  // into a String first - halves the peak heap use of an import.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  return applyConfigurationFromDoc(doc);
}

// ---------------------------------------------------------------------------
// MQTT DataPoints only (0.9.30)
//
// Separate export/import that carries ONLY the MQTT DataPoint definitions, so
// they can be moved between InfoTerm terminals without overwriting the rest
// of a device's configuration (theme, location, WiFi/MQTT broker, widgets).
// The file uses the same per-value field names as the full backup
// (label/topic/unit/decimals/enabled) plus a distinct "type" marker so a full
// backup can never be mistaken for a DataPoint file and vice versa.
// ---------------------------------------------------------------------------
static const char* MQTT_DP_FILE_TYPE = "infoterm-mqtt-datapoints";

bool serializeMqttDataPointsToJson(String& jsonOut) {
  JsonDocument doc;
  doc["type"] = MQTT_DP_FILE_TYPE;
  doc["schemaVersion"] = InfoTermConfigSchema::CURRENT;
  doc["firmwareVersion"] = INFOTERM_APP_VERSION;
  doc["count"] = mqttValueCount;
  JsonArray values = doc["values"].to<JsonArray>();
  for (int i = 0; i < mqttValueCount && i < MQTT_VALUE_COUNT; i++) {
    JsonObject v = values.add<JsonObject>();
    v["label"] = mqttLabels[i];
    v["topic"] = mqttTopics[i];
    v["unit"] = mqttUnits[i];
    v["decimals"] = mqttDecimals[i];
    v["enabled"] = mqttValueEnabled[i];
  }
  serializeJson(doc, jsonOut);
  return true;
}

bool serializeMqttDataPointsToFile(const char* path) {
  String json;
  if (!serializeMqttDataPointsToJson(json)) return false;
  if (!mountConfigFilesystem(true)) return false;
  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = file.print(json);
  file.close();
  return written == json.length();
}

// Applies MQTT DataPoints from a parsed document. Rejects any file without
// the DataPoint type marker (e.g. a full backup) so an import can only ever
// touch the MQTT slots, never the rest of the config. Overwrites all slots:
// the imported count wins, slots beyond it are reset to defaults, so two
// terminals end up with identical DataPoint sets. Does NOT persist or reload
// - the WebGUI done-handler does that (same split as handleMqttDelete).
static bool applyMqttDataPointsFromDoc(JsonDocument& doc) {
  const char* type = doc["type"];
  if (!type || strcmp(type, MQTT_DP_FILE_TYPE) != 0) return false;
  uint16_t storedSchema = doc["schemaVersion"] | 0;
  if (storedSchema == 0 || storedSchema > InfoTermConfigSchema::CURRENT) return false;
  JsonArray values = doc["values"];
  if (values.isNull()) return false;

  int count = constrain((int)(doc["count"] | (int)values.size()), 1, MQTT_VALUE_MAX);
  int i = 0;
  for (JsonObject v : values) {
    if (i >= count || i >= MQTT_VALUE_COUNT) break;
    String label = v["label"].as<String>();
    label.trim();
    if (label.length() == 0) label = "MQTT " + String(i + 1);
    if (label.length() > 18) label = label.substring(0, 18);
    String topic = v["topic"].as<String>();
    topic.trim();
    if (topic.length() > 120) topic = topic.substring(0, 120);
    String unit = v["unit"].as<String>();
    unit.trim();
    if (unit.length() > 8) unit = unit.substring(0, 8);
    mqttLabels[i] = label;
    mqttTopics[i] = topic;
    mqttUnits[i] = unit;
    mqttDecimals[i] = constrain((int)(v["decimals"] | MQTT_DECIMALS_AUTO), MQTT_DECIMALS_AUTO, 4);
    mqttValueEnabled[i] = v["enabled"] | false;
    mqttValues[i] = "--";
    lastMqttLine[i] = "";
    i++;
  }
  int applied = i;
  if (applied < 1) return false;
  // Reset the remaining slots to defaults so the target terminal mirrors the
  // source exactly rather than keeping stale higher-index DataPoints.
  for (int j = applied; j < MQTT_VALUE_COUNT; j++) {
    mqttLabels[j] = "MQTT " + String(j + 1);
    mqttTopics[j] = "";
    mqttUnits[j] = "";
    mqttDecimals[j] = MQTT_DECIMALS_AUTO;
    mqttValueEnabled[j] = false;
    mqttValues[j] = "--";
    lastMqttLine[j] = "";
  }
  mqttValueCount = applied;
  sanitizeWidgetSlotsForMqttCount();
  persistRuntimeConfiguration();
  return true;
}

bool deserializeMqttDataPointsFromFile(const char* path) {
  if (!mountConfigFilesystem(false)) return false;
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  return applyMqttDataPointsFromDoc(doc);
}

// ---------------------------------------------------------------------------
// Scoped export/import of ONLY the design settings (Design tab: colors,
// night mode, widget border/font, rotation) plus the custom tab
// configuration (1.0.11, issue #8) - to share the visual setup between
// terminals without touching DataPoints, network or anything else on the
// Settings tab. Own type marker so a full backup or a DataPoints file is
// rejected, mirroring the MQTT DataPoints export above. Tab widget slots
// reference DataPoints by key and RSS feeds by index only - the referenced
// definitions themselves are deliberately NOT included (same philosophy as
// the excluded DataPoints); missing targets degrade gracefully on the
// importing device.
// ---------------------------------------------------------------------------
static const char* DESIGN_TABS_FILE_TYPE = "infoterm-design-tabs";

bool serializeDesignTabsToJson(String& jsonOut) {
  JsonDocument doc;
  doc["type"] = DESIGN_TABS_FILE_TYPE;
  doc["schemaVersion"] = InfoTermConfigSchema::CURRENT;
  doc["firmwareVersion"] = INFOTERM_APP_VERSION;

  JsonObject design = doc["design"].to<JsonObject>();
  design["accent"] = accentColorKey;
  design["border"] = borderColorKey;
  design["bg"] = backgroundColorKey;
  design["text"] = textColorKey;
  design["menuBg"] = menuBgColorKey;
  design["buttonBg"] = buttonBgColorKey;
  design["widgetBg"] = widgetBgColorKey;
  design["widgetBorder"] = widgetBordersEnabled;
  design["widgetFont"] = widgetFontKey;
  design["nightMode"] = nightModeEnabled;
  design["nightForce"] = nightModeForced;
  design["nightBrightPct"] = nightBrightnessPct;
  design["rotate180"] = displayRotated180;

  // Home widget slots belong to the visual setup too (first user test of
  // the feature: widgets changed on the Home panel were not restored, which
  // reads as "widget selection not restored" - the WebGUI presents Home as
  // just another tab).
  JsonArray homeSlots = doc["homeSlots"].to<JsonArray>();
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    homeSlots.add(widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]));
  }

  JsonArray tabs = doc["tabs"].to<JsonArray>();
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    JsonObject tab = tabs.add<JsonObject>();
    tab["name"] = customTabNames[t];
    tab["enabled"] = customTabEnabled[t];
    tab["content"] = customTabContent[t];
    tab["feed"] = customTabFeedIndex[t];
    JsonArray slots = tab["slots"].to<JsonArray>();
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      slots.add(widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]));
    }
  }

  serializeJson(doc, jsonOut);
  return true;
}

bool serializeDesignTabsToFile(const char* path) {
  String json;
  if (!serializeDesignTabsToJson(json)) return false;
  if (!mountConfigFilesystem(true)) return false;
  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = file.print(json);
  file.close();
  return written == json.length();
}

// Applies design + tab settings from a parsed document. Rejects any file
// without the design-tabs type marker. Applies the color keys to the live
// palette immediately (the full-backup path leaves that to its reboot-heavy
// callers; this import must be visible right away), re-evaluates the night
// overlay and persists.
static bool applyDesignTabsFromDoc(JsonDocument& doc) {
  const char* type = doc["type"];
  if (!type || strcmp(type, DESIGN_TABS_FILE_TYPE) != 0) return false;
  uint16_t storedSchema = doc["schemaVersion"] | 0;
  if (storedSchema == 0 || storedSchema > InfoTermConfigSchema::CURRENT) return false;

  JsonObject design = doc["design"];
  if (design.isNull()) return false;

  accentColorKey = design["accent"] | accentColorKey;
  borderColorKey = design["border"] | borderColorKey;
  backgroundColorKey = design["bg"] | backgroundColorKey;
  textColorKey = design["text"] | textColorKey;
  menuBgColorKey = design["menuBg"] | menuBgColorKey;
  buttonBgColorKey = design["buttonBg"] | buttonBgColorKey;
  widgetBgColorKey = design["widgetBg"] | widgetBgColorKey;
  widgetBordersEnabled = design["widgetBorder"] | widgetBordersEnabled;
  widgetFontKey = design["widgetFont"] | widgetFontKey;
  nightModeEnabled = design["nightMode"] | nightModeEnabled;
  nightModeForced = design["nightForce"] | nightModeForced;
  nightBrightnessPct = constrain((int)(design["nightBrightPct"] | nightBrightnessPct), 10, 100);
  displayRotated180 = design["rotate180"] | displayRotated180;

  // Optional for compatibility with files exported before the home slots
  // were added - absent array keeps the current home widgets.
  JsonArray homeSlots = doc["homeSlots"];
  if (!homeSlots.isNull()) {
    int i = 0;
    for (JsonVariant slot : homeSlots) {
      if (i >= HOME_SLOT_COUNT) break;
      applyWidgetSelectionKey(slot.as<String>(), homeWidgetSlots[i], homeWidgetDataPointIds[i]);
      i++;
    }
  }

  JsonArray tabs = doc["tabs"];
  if (!tabs.isNull()) {
    int t = 0;
    for (JsonObject tab : tabs) {
      if (t >= CUSTOM_TAB_COUNT) break;
      customTabNames[t] = tab["name"] | customTabNames[t];
      customTabEnabled[t] = tab["enabled"] | customTabEnabled[t];
      customTabContent[t] = tab["content"] | customTabContent[t];
      customTabFeedIndex[t] = tab["feed"] | customTabFeedIndex[t];
      JsonArray slots = tab["slots"];
      if (!slots.isNull()) {
        int i = 0;
        for (JsonVariant slot : slots) {
          if (i >= TAB_WIDGET_COUNT) break;
          applyWidgetSelectionKey(slot.as<String>(), customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]);
          i++;
        }
      }
      t++;
    }
  }

  sanitizeWidgetSlotsForMqttCount();
  sanitizeGeneralConfig();

  // Make the imported look take effect immediately (same sequence as the
  // apply section of handleSave).
  applyThemeByKey(accentColorKey, backgroundColorKey);
  applyMenuBgColorByKey(menuBgColorKey);
  applyButtonBgColorByKey(buttonBgColorKey);
  applyWidgetBgColorByKey(widgetBgColorKey);
  applyBorderColorByKey(borderColorKey);
  applyTextColorByKey(textColorKey);
  nightModeActive = shouldNightBeActive();
  if (nightModeActive) applyNightModeColors();
  if (!sleepDimmed && !sleepOff) setBacklight(awakeBacklightLevel());
  applyDisplayRotation();

  persistRuntimeConfiguration();
  pageDirty = true;
  dataDirty = true;
  return true;
}

bool deserializeDesignTabsFromFile(const char* path) {
  if (!mountConfigFilesystem(false)) return false;
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  return applyDesignTabsFromDoc(doc);
}

// ---------------------------------------------------------------------------
// Scoped export/import of ONLY the RSS feed URL list (1.0.11), analogous to
// the MQTT DataPoints export above: own type marker, everything else is
// rejected. Which tab shows which feed is part of the design+tabs export.
// ---------------------------------------------------------------------------
static const char* RSS_FEEDS_FILE_TYPE = "infoterm-rss-feeds";

bool serializeRssFeedsToJson(String& jsonOut) {
  JsonDocument doc;
  doc["type"] = RSS_FEEDS_FILE_TYPE;
  doc["schemaVersion"] = InfoTermConfigSchema::CURRENT;
  doc["firmwareVersion"] = INFOTERM_APP_VERSION;
  doc["count"] = rssFeedCount;
  JsonArray feeds = doc["feeds"].to<JsonArray>();
  for (int i = 0; i < rssFeedCount && i < RSS_FEED_COUNT; i++) {
    feeds.add(rssFeedUrls[i]);
  }
  serializeJson(doc, jsonOut);
  return true;
}

bool serializeRssFeedsToFile(const char* path) {
  String json;
  if (!serializeRssFeedsToJson(json)) return false;
  if (!mountConfigFilesystem(true)) return false;
  File file = LittleFS.open(path, "w");
  if (!file) return false;
  size_t written = file.print(json);
  file.close();
  return written == json.length();
}

// Replaces the stored feed list with the file's list (the imported set
// wins, like the DataPoints import), resets all fetch state and clamps the
// tab feed references onto the new list.
static bool applyRssFeedsFromDoc(JsonDocument& doc) {
  const char* type = doc["type"];
  if (!type || strcmp(type, RSS_FEEDS_FILE_TYPE) != 0) return false;
  uint16_t storedSchema = doc["schemaVersion"] | 0;
  if (storedSchema == 0 || storedSchema > InfoTermConfigSchema::CURRENT) return false;
  JsonArray feeds = doc["feeds"];
  if (feeds.isNull()) return false;

  int i = 0;
  for (JsonVariant v : feeds) {
    if (i >= RSS_FEED_COUNT) break;
    String url = v.as<String>();
    url.trim();
    if (url.length() > 200) url = url.substring(0, 200);
    if (url.length() > 0 && !url.startsWith("http://") && !url.startsWith("https://")) url = "";
    rssFeedUrls[i++] = url;
  }
  rssFeedCount = i;
  for (int k = 0; k < RSS_FEED_COUNT; k++) {
    if (k >= rssFeedCount) rssFeedUrls[k] = "";
    rssResetFeedState(k);
  }
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    if (customTabFeedIndex[t] >= rssFeedCount) customTabFeedIndex[t] = rssFeedCount > 0 ? rssFeedCount - 1 : 0;
  }

  sanitizeGeneralConfig();
  persistRuntimeConfiguration();
  pageDirty = true;
  dataDirty = true;
  return true;
}

bool deserializeRssFeedsFromFile(const char* path) {
  if (!mountConfigFilesystem(false)) return false;
  if (!LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;
  return applyRssFeedsFromDoc(doc);
}
