// InfoTerm Storage persist runtime migration
//
// Persists runtime configuration to ESP32 Preferences/NVS.
// Existing key migration behavior is preserved unchanged.

#include "../config/ConfigSchema.h"

void persistGeneralConfiguration() {
  cfgPutString("accent", accentColorKey);
  cfgPutString("border", borderColorKey);
  cfgPutString("bg", backgroundColorKey);
  cfgPutString("text", textColorKey);
  cfgPutString("menuBg", menuBgColorKey);
  cfgPutString("buttonBg", buttonBgColorKey);
  cfgPutString("widgetBg", widgetBgColorKey);
  cfgPutBool("widgetBorder", widgetBordersEnabled);
  cfgPutString("units", unitKey);
  cfgPutString("region", regionFormatKey);
  cfgPutString("lang", languageCode());
  cfgPutString("tz", timezoneKey);
  cfgPutString("deviceName", deviceName);
  cfgPutString("locname", locationName);
  cfgPutFloat("lat", LAT);
  cfgPutFloat("lng", LNG);
  cfgPutInt("sleepMin", sleepIntervalMin);
  cfgPutInt("logLevel", logLevelValue);

  // Shadow-write critical general settings to explicit short keys.
  // This avoids old migration paths overriding location/sleep settings after restart.
  prefs.putString(CFG_LOCATION_NAME_KEY, locationName);
  prefs.putFloat(CFG_LATITUDE_KEY, LAT);
  prefs.putFloat(CFG_LONGITUDE_KEY, LNG);
  prefs.putInt(CFG_SLEEP_MIN_KEY, sleepIntervalMin);
  cfgPutBool("flashMode", flashModeEnabled);
  cfgPutBool("wifiEnabled", wifiEnabled);
  cfgPutInt("wifiActiveIdx", wifiActiveIndex);
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
    cfgPutString(String("wifiSsid") + String(i), wifiSlotSsid[i]);
    cfgPutString(String("wifiPass") + String(i), wifiSlotPass[i]);
  }
  cfgPutString("webUser", webUser);
  cfgPutString("webPass", webPass);
  cfgPutString("ipMode", ipMode);
  cfgPutString("ipStatic", staticIp);
  cfgPutString("ipGw", staticGateway);
  cfgPutString("ipMask", staticSubnet);
  cfgPutString("ipDns", staticDns);
  cfgPutString("vpnPrivKey", vpnPrivateKey);
  cfgPutString("vpnPeerKey", vpnPeerPublicKey);
  cfgPutString("vpnHost", vpnEndpointHost);
  cfgPutInt("vpnPort", vpnEndpointPort);
  cfgPutString("vpnLocalIp", vpnLocalAddress);
  cfgPutString("vpnDns", vpnDns);
  cfgPutString("vpnAllowed", vpnAllowedIps);
  cfgPutString("vpnName", vpnName);
  cfgPutBool("nightMode", nightModeEnabled);
  cfgPutBool("nightForce", nightModeForced);
  cfgPutInt("nightBrightPct", nightBrightnessPct);
  cfgPutBool("rotate180", displayRotated180);
  cfgPutBool("tabCycle", tabCycleEnabled);
  cfgPutInt("tabCycleSec", tabCycleIntervalSec);
  cfgPutBool("mqttOn", mqttWeatherEnabled);
  cfgPutBool("mqttFallback", mqttFallbackOpenMeteo);
  cfgPutString("mqttHost", mqttHost);
  cfgPutInt("mqttPort", mqttPort);
  cfgPutString("mqttUser", mqttUser);
  cfgPutString("mqttPass", mqttPass);
  cfgPutString("mqttTopic", mqttWeatherflowTopic);
  markConfigSaved();
}

void persistMqttDataPointConfig() {
  cfgPutInt("mqttValueCount", mqttValueCount);

  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    String idx = String(i);
    String labelKey = String("mqttLabel") + idx;
    String topicKey = String("mqttTopic") + idx;
    String unitKeyName = String("mqttUnit") + idx;
    String decimalsKey = String("mqttDec") + idx;
    String enabledKey = String("mqttValEn") + idx;

    cfgPutString(labelKey, mqttLabels[i]);
    cfgPutString(topicKey, mqttTopics[i]);
    cfgPutString(unitKeyName, mqttUnits[i]);
    cfgPutInt(decimalsKey, mqttDecimals[i]);
    cfgPutBool(enabledKey, mqttValueEnabled[i]);

  }
  markConfigSaved();
}

void persistWidgetConfiguration() {
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    String key = String("homeSlot") + String(i);
    cfgPutString(key, widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]));
  }
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabIdx = String(t);
    String tabNameKey = String("tabName") + tabIdx;
    String tabEnabledKey = String("tabEnabled") + tabIdx;
    cfgPutString(tabNameKey, customTabNames[t]);
    cfgPutBool(tabEnabledKey, customTabEnabled[t]);

    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      String key = String("tab") + tabIdx + String("Slot") + String(i);
      cfgPutString(key, widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]));
    }
  }
  markConfigSaved();
}

void persistRuntimeConfiguration() {
  persistGeneralConfiguration();
  persistMqttDataPointConfig();
  persistWidgetConfiguration();
  prefs.putUShort(InfoTermConfigSchema::VERSION_KEY, InfoTermConfigSchema::CURRENT);
}
