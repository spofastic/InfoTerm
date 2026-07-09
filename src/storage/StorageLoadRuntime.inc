// InfoTerm Storage load runtime migration
//
// Loads persisted configuration from ESP32 Preferences/NVS.
// Existing key migration behavior is preserved unchanged.

#include "../config/ConfigSchema.h"
#include "../config/FirmwareMigrationBoot.h"

void cleanupLegacyConfigKeys();

// Shared plausibility/normalization pass for fields that can end up with an
// out-of-range or foreign value either from NVS (e.g. an older/foreign
// schema) or from a Backup/Restore JSON import - both loadStoredSettings()
// and deserializeConfigurationFromJson() (StorageBackupRuntime.inc) call this
// after populating the runtime globals, so a corrupted or hand-edited backup
// file cannot leave an invalid value in place until the next reboot.
void sanitizeGeneralConfig() {
  if (unitKey != "metric" && unitKey != "imperial") unitKey = "metric";
  if (regionFormatKey != "europe" && regionFormatKey != "us") regionFormatKey = "europe";

  mqttHost.trim();
  mqttUser.trim();
  mqttPass.trim();
  mqttWeatherflowTopic.trim();
  if (mqttPort == 0) mqttPort = DEFAULT_MQTT_PORT;
  if (mqttWeatherflowTopic.length() == 0) mqttWeatherflowTopic = DEFAULT_MQTT_TOPIC_WEATHERFLOW_OBSERVATION;

  if (!(LAT >= -90.0f && LAT <= 90.0f)) LAT = 52.5200f;
  if (!(LNG >= -180.0f && LNG <= 180.0f)) LNG = 13.4050f;

  // 0 is a valid, intentional value (auto-sleep disabled, see
  // handleAutoSleep() in SystemRuntime.inc) - only clamp the upper bound and
  // reject negative values.
  sleepIntervalMin = constrain(sleepIntervalMin, 0, 1440);

  if (tabCycleIntervalSec != 30 && tabCycleIntervalSec != 60 &&
      tabCycleIntervalSec != 180 && tabCycleIntervalSec != 360) {
    tabCycleIntervalSec = 60;
  }

  wifiActiveIndex = constrain(wifiActiveIndex, 0, WIFI_NETWORK_COUNT - 1);

  if (vpnEndpointPort == 0) vpnEndpointPort = 51820;
}

void loadStoredSettings() {
  prefs.begin("infoterm", false);

  uint16_t storedSchemaVersion = prefs.getUShort(InfoTermConfigSchema::VERSION_KEY, 0);
  InfoTermFirmwareMigrationBoot::begin(storedSchemaVersion);
  if (InfoTermFirmwareMigrationBoot::needsFactoryReset()) {
    Logger::error("[Boot] Stored config schema is newer than this firmware; keeping existing values, no automatic reset.");
  }

  String accent = cfgGetString("accent", "accent", "cyan");
  String border = cfgGetString("border", "border", "cyan");
  if (border == "theme" || border.length() == 0) border = "cyan";
  String bg     = cfgGetString("bg", "bg", "petrol");
  String txt    = cfgGetString("text", "text", "standard");
  String menuBg = cfgGetString("menuBg", "menuBg", "theme");
  String buttonBg = cfgGetString("buttonBg", "buttonBg", "theme");
  String widgetBg = cfgGetString("widgetBg", "widgetBg", "theme");
  widgetBordersEnabled = cfgGetBool("widgetBorder", "widgetBorder", true);

  deviceName    = cfgGetString("deviceName", "deviceName", "");
  locationName     = prefs.isKey(CFG_LOCATION_NAME_KEY) ? prefs.getString(CFG_LOCATION_NAME_KEY, "Berlin") : cfgGetString("locname", "locname", "Berlin");
  LAT              = prefs.isKey(CFG_LATITUDE_KEY) ? prefs.getFloat(CFG_LATITUDE_KEY, 52.5200f) : cfgGetFloat("lat", "lat", 52.5200f);
  LNG              = prefs.isKey(CFG_LONGITUDE_KEY) ? prefs.getFloat(CFG_LONGITUDE_KEY, 13.4050f) : cfgGetFloat("lng", "lng", 13.4050f);
  sleepIntervalMin = prefs.isKey(CFG_SLEEP_MIN_KEY) ? prefs.getInt(CFG_SLEEP_MIN_KEY, 10) : cfgGetInt("sleepMin", "sleepMin", 10);
  unitKey          = cfgGetString("units", "units", "metric");
  regionFormatKey  = cfgGetString("region", "region", "europe");
  uiLanguage       = languageFromCode(cfgGetString("lang", "lang", "de"));
  timezoneKey      = sanitizeTimezoneKey(cfgGetString("tz", "tz", "europe_central"));
  flashModeEnabled = cfgGetBool("flashMode", "flashMode", false);
  wifiEnabled      = cfgGetBool("wifiEnabled", "wifiEnabled", true);
  wifiActiveIndex  = cfgGetInt("wifiActiveIdx", "wifiActiveIdx", 0);
  ipMode           = cfgGetString("ipMode", "ipMode", INFOTERM_IP_MODE);
  staticIp         = cfgGetString("ipStatic", "ipStatic", INFOTERM_STATIC_IP);
  staticGateway    = cfgGetString("ipGw", "ipGw", INFOTERM_STATIC_GATEWAY);
  staticSubnet     = cfgGetString("ipMask", "ipMask", INFOTERM_STATIC_SUBNET);
  staticDns        = cfgGetString("ipDns", "ipDns", INFOTERM_STATIC_DNS);
  vpnPrivateKey    = cfgGetString("vpnPrivKey", "vpnPrivKey", "");
  vpnPeerPublicKey = cfgGetString("vpnPeerKey", "vpnPeerKey", "");
  vpnEndpointHost  = cfgGetString("vpnHost", "vpnHost", "");
  vpnEndpointPort  = (uint16_t)cfgGetInt("vpnPort", "vpnPort", 51820);
  vpnLocalAddress  = cfgGetString("vpnLocalIp", "vpnLocalIp", "");
  vpnDns           = cfgGetString("vpnDns", "vpnDns", "");
  vpnAllowedIps    = cfgGetString("vpnAllowed", "vpnAllowed", "");
  vpnName          = cfgGetString("vpnName", "vpnName", "");
  nightModeEnabled = cfgGetBool("nightMode", "nightMode", false);
  displayRotated180 = cfgGetBool("rotate180", "rotate180", false);
  tabCycleEnabled = cfgGetBool("tabCycle", "tabCycle", false);
  tabCycleIntervalSec = cfgGetInt("tabCycleSec", "tabCycleSec", 60);
  logLevelValue    = constrain(cfgGetInt("logLevel", "logLevel", static_cast<int>(Logger::Level::Info)), 0, 3);
  Logger::setLevel(logLevelValue);
  Logger::debug(String("Configured log level: ") + Logger::levelName(Logger::level()));
  mqttWeatherEnabled = cfgGetBool("mqttOn", "mqttOn", DEFAULT_MQTT_WEATHER_ENABLED);
  mqttFallbackOpenMeteo = cfgGetBool("mqttFallback", "mqttFallback", DEFAULT_MQTT_FALLBACK_OPENMETEO);
  mqttHost = cfgGetString("mqttHost", "mqttHost", DEFAULT_MQTT_HOST);
  mqttPort = (uint16_t)cfgGetInt("mqttPort", "mqttPort", DEFAULT_MQTT_PORT);
  mqttUser = cfgGetString("mqttUser", "mqttUser", DEFAULT_MQTT_USER);
  mqttPass = cfgGetString("mqttPass", "mqttPass", DEFAULT_MQTT_PASS);
  mqttWeatherflowTopic = cfgGetString("mqttTopic", "mqttTopic", DEFAULT_MQTT_TOPIC_WEATHERFLOW_OBSERVATION);
  mqttValueCount = constrain(cfgGetInt("mqttValueCount", "mqttValueCount", MQTT_DEFAULT_VALUE_COUNT), 1, MQTT_VALUE_MAX);
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    String idx = String(i);
    String labelKey = String("mqttLabel") + idx;
    String topicKey = String("mqttTopic") + idx;
    String unitKeyName = String("mqttUnit") + idx;
    String decimalsKey = String("mqttDec") + idx;
    String enabledKey = String("mqttValEn") + idx;
    mqttLabels[i] = cfgGetString(labelKey, labelKey, mqttLabels[i]);
    mqttTopics[i] = cfgGetString(topicKey, topicKey, "");
    mqttUnits[i] = cfgGetString(unitKeyName, unitKeyName, "");
    mqttDecimals[i] = constrain(cfgGetInt(decimalsKey, decimalsKey, MQTT_DECIMALS_AUTO), MQTT_DECIMALS_AUTO, 4);
    mqttValueEnabled[i] = cfgGetBool(enabledKey, enabledKey, false);
    mqttLabels[i].trim();
    mqttTopics[i].trim();
    mqttUnits[i].trim();
    if (mqttLabels[i].length() == 0) mqttLabels[i] = "MQTT " + String(i + 1);
    if (mqttUnits[i].length() > 8) mqttUnits[i] = mqttUnits[i].substring(0, 8);
  }

  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    String key = String("homeSlot") + String(i);
    String storedSelection = cfgGetString(key, key, widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]));
    applyWidgetSelectionKey(storedSelection, homeWidgetSlots[i], homeWidgetDataPointIds[i]);
  }

  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabIdx = String(t);
    customTabNames[t] = cfgGetString(String("tabName") + tabIdx, String("tabName") + tabIdx, customTabNames[t]);
    customTabNames[t].trim();
    if (customTabNames[t].length() == 0) customTabNames[t] = "Tab " + String(t + 1);
    if (customTabNames[t].length() > 10) customTabNames[t] = customTabNames[t].substring(0, 10);
    customTabEnabled[t] = cfgGetBool(String("tabEnabled") + tabIdx, String("tabEnabled") + tabIdx, customTabEnabled[t]);
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      String key = String("tab") + tabIdx + String("Slot") + String(i);
      String storedSelection = cfgGetString(key, key, widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]));
      applyWidgetSelectionKey(storedSelection, customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]);
    }
  }

  sanitizeWidgetSlotsForMqttCount();
  sanitizeGeneralConfig();
  deviceName.trim();
  applyThemeByKey(accent, bg);
  applyMenuBgColorByKey(menuBg);
  applyButtonBgColorByKey(buttonBg);
  applyWidgetBgColorByKey(widgetBg);
  applyBorderColorByKey(border);
  applyTextColorByKey(txt);
  applyDeviceTimezoneByKey(timezoneKey);

  // sunriseMin/sunsetMin are still -1 this early (no weather fetch yet), so
  // isNightNow() will be false right after boot even during a real night
  // window; checkNightModeTransition() (schedulerWeatherUpdate) corrects
  // this within DATA_TICK_MS of the first successful weather fetch.
  nightModeActive = nightModeEnabled && isNightNow();
  if (nightModeActive) applyNightModeColors();

  // If this device only has legacy Preference keys, migrate the currently
  // loaded configuration into the version-independent cfg_ keys.
  if (!cfgHasSavedConfig()) {
    persistRuntimeConfiguration();
  }

  cleanupLegacyConfigKeys();
}

// One-time cleanup of legacy cfg_*/cfg2_* keys once the current hashed-key
// generation holds the config. These were never removed after migrating to
// the next key generation, and with ~250 individual settings (32 MQTT slots
// x 5 fields, widget/tab slots, ...) stacked across three key generations,
// the tiny stock nvs partition ran out of free pages - which makes the
// ESP32 core silently erase+reformat all of nvs on the next boot.
void cleanupLegacyConfigKeys() {
  static const char* CONFIG_LEGACY_CLEANED_MARKER = "c3clean";
  if (prefs.getBool(CONFIG_LEGACY_CLEANED_MARKER, false)) return;
  if (!cfgHasSavedConfig()) return;

  // isKey() checks are silent; remove() itself always logs an [E] line via
  // the ESP-IDF core's log_e() on NOT_FOUND, even though that's the expected
  // outcome for almost every key here on a device that never had them. Only
  // calling remove() when the key actually exists avoids ~450 noisy log
  // lines (and their Serial-print time) on the one-time first run.
  auto removeLegacy = [](const String& base) {
    String key1 = legacyCfgKey(base);
    if (prefs.isKey(key1.c_str())) prefs.remove(key1.c_str());
    String key2 = oldCfg2Key(base);
    if (prefs.isKey(key2.c_str())) prefs.remove(key2.c_str());
  };

  static const char* staticKeys[] = {
    "accent", "border", "bg", "text", "menuBg", "buttonBg", "widgetBg",
    "widgetBorder", "units", "region", "lang", "tz", "deviceName", "locname",
    "lat", "lng", "sleepMin", "logLevel", "flashMode", "wifiEnabled",
    "mqttOn", "mqttFallback", "mqttHost", "mqttPort", "mqttUser", "mqttPass",
    "mqttTopic", "mqttValueCount"
  };
  for (const char* key : staticKeys) removeLegacy(key);

  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    String idx = String(i);
    removeLegacy(String("mqttLabel") + idx);
    removeLegacy(String("mqttTopic") + idx);
    removeLegacy(String("mqttUnit") + idx);
    removeLegacy(String("mqttDec") + idx);
    removeLegacy(String("mqttValEn") + idx);
  }

  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    removeLegacy(String("homeSlot") + String(i));
  }

  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabIdx = String(t);
    removeLegacy(String("tabName") + tabIdx);
    removeLegacy(String("tabEnabled") + tabIdx);
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      removeLegacy(String("tab") + tabIdx + String("Slot") + String(i));
    }
  }

  prefs.putBool(CONFIG_LEGACY_CLEANED_MARKER, true);
}
