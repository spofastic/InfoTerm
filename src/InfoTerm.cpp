/******************************************************************************
 * InfoTerm
 *
 * Universal ESP32 information dashboard for weather, MQTT and system status.
 *
 * This module contains the current legacy firmware implementation and is the
 * root of the unity build (globals first, then the .inc runtime modules).
 * Application entry points are separated in Application.inc. Formerly
 * src/InfoTerm.ino - converted to a plain .cpp in 0.9.26; the sketch
 * preprocessor added nothing here beyond this Arduino.h include (the unity
 * build was already declaration-ordered, verified by a green first build).
 ******************************************************************************/
#include <Arduino.h>
#include <FS.h>
using fs::FS;

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include "include/Config.h"
#include "include/InfoTermLogic.h"
#include "include/Language.h"
#include "include/Version.h"
#include "system/Logger.h"
#include "include/Types.h"
#include "include/DataPointManager.h"

// =========================================================
// MQTT / HOME ASSISTANT WETTERSTATION
// =========================================================
// MQTT-Wetter kann ueber die InfoTerm-Weboberflaeche konfiguriert werden.
// Standardwerte passen zum WeatherFlow2MQTT-Topic aus dem MQTT-Explorer.
static const bool DEFAULT_MQTT_WEATHER_ENABLED = true;
static const bool DEFAULT_MQTT_FALLBACK_OPENMETEO = true;
static const char* DEFAULT_MQTT_HOST = "192.168.178.10";
static const uint16_t DEFAULT_MQTT_PORT = 1883;
static const char* DEFAULT_MQTT_USER = "mqtt_user";
static const char* DEFAULT_MQTT_PASS = "mqtt_password";
static const char* MQTT_CLIENT_ID = "InfoTerm";

static const char* DEFAULT_MQTT_TOPIC_WEATHERFLOW_OBSERVATION = "homeassistant/sensor/weatherflow2mqtt_ST-00166906/observation/state";

// Optionale Ersatz-Topics. Bei WeatherFlow2MQTT koennen diese ignoriert werden.
static const char* MQTT_TOPIC_WEATHER_JSON = "infoterm/weather/json";
static const char* MQTT_TOPIC_TEMP         = "infoterm/weather/temperature";
static const char* MQTT_TOPIC_TEMP_MIN     = "infoterm/weather/temperature_min";
static const char* MQTT_TOPIC_TEMP_MAX     = "infoterm/weather/temperature_max";
static const char* MQTT_TOPIC_RAIN         = "infoterm/weather/rain";
static const char* MQTT_TOPIC_WIND_SPEED   = "infoterm/weather/wind_speed";
static const char* MQTT_TOPIC_WIND_DIR     = "infoterm/weather/wind_direction";
static const char* MQTT_TOPIC_UV           = "infoterm/weather/uv";
static const char* MQTT_TOPIC_HUMIDITY     = "infoterm/weather/humidity";
static const char* MQTT_TOPIC_PRESSURE     = "infoterm/weather/pressure";

bool mqttWeatherEnabled = DEFAULT_MQTT_WEATHER_ENABLED;
bool mqttFallbackOpenMeteo = DEFAULT_MQTT_FALLBACK_OPENMETEO;
String mqttHost = DEFAULT_MQTT_HOST;
uint16_t mqttPort = DEFAULT_MQTT_PORT;
String mqttUser = DEFAULT_MQTT_USER;
String mqttPass = DEFAULT_MQTT_PASS;
String mqttWeatherflowTopic = DEFAULT_MQTT_TOPIC_WEATHERFLOW_OBSERVATION;
bool mqttWasVerbunden = false;
UiLanguage uiLanguage = LANG_DE;

// =========================================================
// DISPLAY / TOUCH
// =========================================================
TFT_eSPI tft;

const int ROT = 2;
const bool INV = false;

// Persisted 180-degree flip on top of the base orientation ROT (Design tab,
// for upside-down mounted devices). The touch controller deliberately stays
// on ts.setRotation(ROT) so the raw calibration values (TOUCH_X_MIN etc.)
// keep applying unchanged; readTouchXY() mirrors the mapped screen
// coordinates instead when the flip is active.
bool displayRotated180 = false;

void applyDisplayRotation() {
  tft.setRotation(displayRotated180 ? (ROT + 2) % 4 : ROT);
}

#ifndef TOUCH_CS
#define TOUCH_CS  33
#endif
#define TOUCH_IRQ 36

static const int T_SCK  = 25;
static const int T_MISO = 39;
static const int T_MOSI = 32;

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS);

static const int TOUCH_X_MIN = 562;
static const int TOUCH_X_MAX = 3604;
static const int TOUCH_Y_MIN = 544;
static const int TOUCH_Y_MAX = 3720;

static const bool TOUCH_SWAP_XY = false;
static const bool TOUCH_FLIP_X  = false;
static const bool TOUCH_FLIP_Y  = false;

// =========================================================
// WEB / STORAGE
// =========================================================
WebServer server(80);
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
Preferences prefs;
DataPointManager dataPointManager;

#include "storage/StorageConfigRuntime.inc"

// =========================================================
// SPRITES
// =========================================================
TFT_eSprite sprClock = TFT_eSprite(&tft);
TFT_eSprite sprSmall = TFT_eSprite(&tft);

// =========================================================
// LOCATION
// =========================================================
float LAT = 52.5200f;
float LNG = 13.4050f;
String locationName = "Berlin";

// =========================================================
// THEME
// =========================================================
uint16_t COL_BG        = 0x08A3;
uint16_t COL_PANEL     = 0x1106;
uint16_t COL_PANEL_ALT = 0x18C7;
uint16_t COL_STROKE    = 0x31EC;
uint16_t COL_TEXT      = 0xEF7D;
uint16_t COL_DIM       = 0x94B2;
uint16_t COL_ACCENT    = 0x5EFA;
uint16_t COL_MENU_BG   = 0x18C7;
uint16_t COL_BUTTON_BG = 0x1106;


const uint16_t COL_GREEN  = TFT_GREEN;
const uint16_t COL_YELLOW = 0xFFE0;
const uint16_t COL_RED    = TFT_RED;
const uint16_t COL_BLUE   = 0x041F;

String accentColorKey = "cyan";
String backgroundColorKey = "petrol";
String textColorKey = "standard";
String borderColorKey = "cyan";
String menuBgColorKey = "theme";
String buttonBgColorKey = "theme";
String widgetBgColorKey = "theme";
bool widgetBordersEnabled = true;
bool nightModeEnabled = false;  // persisted - user opt-in for the feature
bool nightModeActive = false;   // runtime-only - whether the red overlay is
                                 // currently applied; never persisted, a
                                 // reboot re-derives it (day colors unless
                                 // nightModeForced is set)
int nightBrightnessPct = 25;    // persisted - awake backlight level (percent)
                                 // while the night palette is active; valid
                                 // values 10/25/50/75/100
bool nightModeForced = false;   // persisted - "Nachtmodus erzwingen": night
                                 // palette + dimming active regardless of the
                                 // sunset/sunrise window (also for daytime
                                 // testing of the night settings)
String unitKey = "metric"; // metric = C/mm, imperial = F/in
String regionFormatKey = "europe"; // europe = 24h + dd.mm.yyyy, us = 12h + mm/dd/yyyy
String timezoneKey = "europe_central";

// =========================================================
// LAYOUT
// =========================================================
const int SCREEN_W = 240;
const int SCREEN_H = 320;
const int TOPBAR_H = 0;
const int NAV_H    = 44;

const int HOME_GRID_Y1 = 82;
const int HOME_GRID_Y2 = 146;
const int HOME_GRID_Y3 = 210;
const int HOME_WIDGET_H = 58;

const int CUSTOM_TAB_COUNT = 3;
const int TAB_WIDGET_COUNT = 8;
const int TAB_GRID_Y1 = 6;
const int TAB_GRID_Y2 = 72;
const int TAB_GRID_Y3 = 138;
const int TAB_GRID_Y4 = 204;
const int TAB_WIDGET_H = 58;

const int PAGE_ROW1_Y = 8;
const int PAGE_ROW2_Y = 82;
const int PAGE_ROW3_Y = 156;
const int PAGE_WIDGET_H = 70;

// =========================================================
// MQTT DISPLAY VALUES
// =========================================================
const int MQTT_VALUE_MAX = 32;
const int MQTT_DEFAULT_VALUE_COUNT = 12;
const int MQTT_VALUE_COUNT = MQTT_VALUE_MAX;  // Compatibility constant for fixed storage arrays.
const int MQTT_DECIMALS_AUTO = -1;
int mqttValueCount = MQTT_DEFAULT_VALUE_COUNT;
String mqttLabels[MQTT_VALUE_COUNT] = {"MQTT 1", "MQTT 2", "MQTT 3", "MQTT 4", "MQTT 5", "MQTT 6", "MQTT 7", "MQTT 8", "MQTT 9", "MQTT 10", "MQTT 11", "MQTT 12", "MQTT 13", "MQTT 14", "MQTT 15", "MQTT 16", "MQTT 17", "MQTT 18", "MQTT 19", "MQTT 20", "MQTT 21", "MQTT 22", "MQTT 23", "MQTT 24", "MQTT 25", "MQTT 26", "MQTT 27", "MQTT 28", "MQTT 29", "MQTT 30", "MQTT 31", "MQTT 32"};
String mqttTopics[MQTT_VALUE_COUNT] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
String mqttUnits[MQTT_VALUE_COUNT] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
int mqttDecimals[MQTT_VALUE_COUNT] = {MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO, MQTT_DECIMALS_AUTO};
bool mqttValueEnabled[MQTT_VALUE_COUNT] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
String mqttValues[MQTT_VALUE_COUNT] = {"--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--", "--"};
String lastMqttLine[MQTT_VALUE_COUNT] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
String lastMqttStatusText = "";

String deviceName = "";



const int HOME_SLOT_COUNT = 6;
HomeWidgetType homeWidgetSlots[HOME_SLOT_COUNT] = {
  HOME_WIDGET_WEEK,
  HOME_WIDGET_RAIN,
  HOME_WIDGET_OUTDOOR,
  HOME_WIDGET_WIND,
  HOME_WIDGET_SUN,
  HOME_WIDGET_MQTT_1
};

String cacheHomeSlots[HOME_SLOT_COUNT];
String homeWidgetDataPointIds[HOME_SLOT_COUNT] = {"", "", "", "", "", ""};

String customTabNames[CUSTOM_TAB_COUNT] = {"Wetter", "MQTT", "Tab 3"};
bool customTabEnabled[CUSTOM_TAB_COUNT] = {true, true, false};
HomeWidgetType customTabWidgets[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT] = {
  {HOME_WIDGET_OUTDOOR, HOME_WIDGET_RAIN, HOME_WIDGET_UV, HOME_WIDGET_WIND, HOME_WIDGET_SUN, HOME_WIDGET_KP, HOME_WIDGET_WEEK, HOME_WIDGET_MQTT_1},
  {HOME_WIDGET_MQTT_1, HOME_WIDGET_MQTT_2, HOME_WIDGET_MQTT_3, HOME_WIDGET_MQTT_4, HOME_WIDGET_MQTT_5, HOME_WIDGET_MQTT_6, HOME_WIDGET_MQTT_7, HOME_WIDGET_MQTT_8},
  {HOME_WIDGET_MQTT_5, HOME_WIDGET_MQTT_6, HOME_WIDGET_MQTT_7, HOME_WIDGET_MQTT_8, HOME_WIDGET_MQTT_9, HOME_WIDGET_MQTT_10, HOME_WIDGET_MQTT_11, HOME_WIDGET_MQTT_12}
};
String cacheCustomTabSlots[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT];
String customTabWidgetDataPointIds[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT];

// =========================================================
// STATE
// =========================================================


Page currentPage = PAGE_HOME;
Page lastDrawnPage = (Page)-1;

const unsigned long CLOCK_TICK_MS = 1000;
const unsigned long DATA_TICK_MS  = 30UL * 1000UL;

bool pageDirty = true;
bool dataDirty = true;

// cache
String cacheClock = "";
String cacheTemp = "";
String cacheRain = "";
String cacheWeek = "";
String cacheHomeEmpty1 = "";
String cacheHomeEmpty2 = "";

String lastWifiText = "";
String lastSignalText = "";
String lastIpText = "";
String lastUptimeText = "";
String lastTempText = "";
String lastRainText = "";
String lastUvText = "";
String lastUvLevelText = "";
String lastKpText = "";
String lastKpLevelText = "";
String lastWindText = "";
String lastWindDirText = "";
String lastNextSunLabel = "";
String lastNextSunTime = "";
String lastNetworkToggleText = "";

const char* homeWidgetKey(HomeWidgetType type) {
  switch (type) {
    case HOME_WIDGET_WEEK:    return "week";
    case HOME_WIDGET_RAIN:    return "rain";
    case HOME_WIDGET_OUTDOOR: return "outdoor";
    case HOME_WIDGET_KP:      return "kp";
    case HOME_WIDGET_UV:      return "uv";
    case HOME_WIDGET_WIND:    return "wind";
    case HOME_WIDGET_SUN:     return "sun";
    case HOME_WIDGET_MQTT_1:  return "mqtt1";
    case HOME_WIDGET_MQTT_2:  return "mqtt2";
    case HOME_WIDGET_MQTT_3:  return "mqtt3";
    case HOME_WIDGET_MQTT_4:  return "mqtt4";
    case HOME_WIDGET_MQTT_5:  return "mqtt5";
    case HOME_WIDGET_MQTT_6:  return "mqtt6";
    case HOME_WIDGET_MQTT_7:  return "mqtt7";
    case HOME_WIDGET_MQTT_8:  return "mqtt8";
    case HOME_WIDGET_MQTT_9:  return "mqtt9";
    case HOME_WIDGET_MQTT_10: return "mqtt10";
    case HOME_WIDGET_MQTT_11: return "mqtt11";
    case HOME_WIDGET_MQTT_12: return "mqtt12";
    case HOME_WIDGET_DATAPOINT: return "datapoint";
    default:                  return "week";
  }
}

const char* homeWidgetLabel(HomeWidgetType type) {
  switch (type) {
    case HOME_WIDGET_WEEK:    return TXT(L_WIDGET_WEEK);
    case HOME_WIDGET_RAIN:    return TXT(L_WIDGET_RAIN);
    case HOME_WIDGET_OUTDOOR: return TXT(L_WIDGET_OUTDOOR);
    case HOME_WIDGET_KP:      return TXT(L_WIDGET_KP);
    case HOME_WIDGET_UV:      return TXT(L_WIDGET_UV);
    case HOME_WIDGET_WIND:    return TXT(L_WIDGET_WIND);
    case HOME_WIDGET_SUN:     return TXT(L_WIDGET_SUN);
    case HOME_WIDGET_MQTT_1:  return mqttLabels[0].c_str();
    case HOME_WIDGET_MQTT_2:  return mqttLabels[1].c_str();
    case HOME_WIDGET_MQTT_3:  return mqttLabels[2].c_str();
    case HOME_WIDGET_MQTT_4:  return mqttLabels[3].c_str();
    case HOME_WIDGET_MQTT_5:  return mqttLabels[4].c_str();
    case HOME_WIDGET_MQTT_6:  return mqttLabels[5].c_str();
    case HOME_WIDGET_MQTT_7:  return mqttLabels[6].c_str();
    case HOME_WIDGET_MQTT_8:  return mqttLabels[7].c_str();
    case HOME_WIDGET_MQTT_9:  return mqttLabels[8].c_str();
    case HOME_WIDGET_MQTT_10: return mqttLabels[9].c_str();
    case HOME_WIDGET_MQTT_11: return mqttLabels[10].c_str();
    case HOME_WIDGET_MQTT_12: return mqttLabels[11].c_str();
    case HOME_WIDGET_DATAPOINT: return "Datenpunkt";
    default:                  return TXT(L_WIDGET_WEEK);
  }
}

HomeWidgetType homeWidgetFromKey(const String& key) {
  if (key.startsWith("dp:")) return HOME_WIDGET_DATAPOINT;
  if (key == "week") return HOME_WIDGET_WEEK;
  if (key == "rain") return HOME_WIDGET_RAIN;
  if (key == "outdoor") return HOME_WIDGET_OUTDOOR;
  if (key == "kp") return HOME_WIDGET_KP;
  if (key == "uv") return HOME_WIDGET_UV;
  if (key == "wind") return HOME_WIDGET_WIND;
  if (key == "sun") return HOME_WIDGET_SUN;
  if (key == "mqtt1") return HOME_WIDGET_MQTT_1;
  if (key == "mqtt2") return HOME_WIDGET_MQTT_2;
  if (key == "mqtt3") return HOME_WIDGET_MQTT_3;
  if (key == "mqtt4") return HOME_WIDGET_MQTT_4;
  if (key == "mqtt5") return HOME_WIDGET_MQTT_5;
  if (key == "mqtt6") return HOME_WIDGET_MQTT_6;
  if (key == "mqtt7") return HOME_WIDGET_MQTT_7;
  if (key == "mqtt8") return HOME_WIDGET_MQTT_8;
  if (key == "mqtt9") return HOME_WIDGET_MQTT_9;
  if (key == "mqtt10") return HOME_WIDGET_MQTT_10;
  if (key == "mqtt11") return HOME_WIDGET_MQTT_11;
  if (key == "mqtt12") return HOME_WIDGET_MQTT_12;
  return HOME_WIDGET_WEEK;
}

static String dataPointWidgetKey(const String& dataPointId) {
  return String("dp:") + dataPointId;
}

static String widgetSelectionKey(HomeWidgetType type, const String& dataPointId) {
  if (type == HOME_WIDGET_DATAPOINT && dataPointId.length() > 0) {
    return dataPointWidgetKey(dataPointId);
  }
  return homeWidgetKey(type);
}

static void applyWidgetSelectionKey(const String& selectionKey, HomeWidgetType& type, String& dataPointId) {
  String key = selectionKey;
  key.trim();
  if (key.startsWith("dp:")) {
    String id = key.substring(3);
    id.trim();
    if (id.length() > 0) {
      type = HOME_WIDGET_DATAPOINT;
      dataPointId = id;
      return;
    }
  }
  type = homeWidgetFromKey(key);
  if (type != HOME_WIDGET_DATAPOINT) dataPointId = "";
}

static bool isMqttWidget(HomeWidgetType type) {
  return type >= HOME_WIDGET_MQTT_1 && type <= HOME_WIDGET_MQTT_12;
}

static int mqttWidgetIndex(HomeWidgetType type) {
  if (!isMqttWidget(type)) return -1;
  return (int)type - (int)HOME_WIDGET_MQTT_1;
}

static void sanitizeWidgetSlotsForMqttCount() {
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    int mqttIndex = mqttWidgetIndex(homeWidgetSlots[i]);
    if (mqttIndex >= mqttValueCount) homeWidgetSlots[i] = HOME_WIDGET_WEEK;
    if (homeWidgetSlots[i] != HOME_WIDGET_DATAPOINT) homeWidgetDataPointIds[i] = "";
  }

  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      int mqttIndex = mqttWidgetIndex(customTabWidgets[t][i]);
      if (mqttIndex >= mqttValueCount) customTabWidgets[t][i] = HOME_WIDGET_WEEK;
      if (customTabWidgets[t][i] != HOME_WIDGET_DATAPOINT) customTabWidgetDataPointIds[t][i] = "";
    }
  }
}

const char* homeSlotLabel(int slot) {
  switch (slot) {
    case 0: return TXT(L_POS_TL);
    case 1: return TXT(L_POS_TR);
    case 2: return TXT(L_POS_ML);
    case 3: return TXT(L_POS_MR);
    case 4: return TXT(L_POS_BL);
    case 5: return TXT(L_POS_BR);
    default: return "Widget";
  }
}

const char* tabWidgetSlotLabel(int slot) {
  switch (slot) {
    case 0: return TXT(L_POS_TL);
    case 1: return TXT(L_POS_TR);
    case 2: return TXT(L_POS_UML);
    case 3: return TXT(L_POS_UMR);
    case 4: return TXT(L_POS_LML);
    case 5: return TXT(L_POS_LMR);
    case 6: return TXT(L_POS_BL);
    case 7: return TXT(L_POS_BR);
    default: return "Widget";
  }
}

const char* timezonePosixByKey(const String& key) {
  if (key == "utc") return "UTC0";
  if (key == "atlantic_azores") return "AZOT1AZOST,M3.5.0/0,M10.5.0/1";
  if (key == "europe_west") return "WET0WEST,M3.5.0/1,M10.5.0";
  if (key == "uk") return "GMT0BST,M3.5.0/1,M10.5.0";
  if (key == "europe_central") return "CET-1CEST,M3.5.0/2,M10.5.0/3";
  if (key == "europe_east") return "EET-2EEST,M3.5.0/3,M10.5.0/4";
  if (key == "africa_south") return "SAST-2";
  if (key == "middle_east_gulf") return "GST-4";
  if (key == "india") return "IST-5:30";
  if (key == "thailand") return "ICT-7";
  if (key == "china") return "CST-8";
  if (key == "us_eastern") return "EST5EDT,M3.2.0/2,M11.1.0/2";
  if (key == "us_central") return "CST6CDT,M3.2.0/2,M11.1.0/2";
  if (key == "us_mountain") return "MST7MDT,M3.2.0/2,M11.1.0/2";
  if (key == "us_arizona") return "MST7";
  if (key == "us_pacific") return "PST8PDT,M3.2.0/2,M11.1.0/2";
  if (key == "alaska") return "AKST9AKDT,M3.2.0/2,M11.1.0/2";
  if (key == "hawaii") return "HST10";
  if (key == "canada_atlantic") return "AST4ADT,M3.2.0/2,M11.1.0/2";
  if (key == "brazil_east") return "BRT3";
  if (key == "argentina") return "ART3";
  if (key == "asia_tokyo") return "JST-9";
  if (key == "korea") return "KST-9";
  if (key == "australia_perth") return "AWST-8";
  if (key == "australia_darwin") return "ACST-9:30";
  if (key == "australia_sydney") return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  if (key == "new_zealand") return "NZST-12NZDT,M9.5.0/2,M4.1.0/3";
  return "CET-1CEST,M3.5.0/2,M10.5.0/3";
}

const char* timezoneLabelByKey(const String& key) {
  if (key == "utc") return "UTC";
  if (key == "atlantic_azores") return "Azores";
  if (key == "europe_west") return "Western Europe";
  if (key == "uk") return "United Kingdom";
  if (key == "europe_central") return "Central Europe";
  if (key == "europe_east") return "Eastern Europe";
  if (key == "africa_south") return "South Africa";
  if (key == "middle_east_gulf") return "Gulf / UAE";
  if (key == "india") return "India";
  if (key == "thailand") return "Thailand / ICT";
  if (key == "china") return "China";
  if (key == "us_eastern") return "US Eastern";
  if (key == "us_central") return "US Central";
  if (key == "us_mountain") return "US Mountain";
  if (key == "us_arizona") return "US Arizona";
  if (key == "us_pacific") return "US Pacific";
  if (key == "alaska") return "Alaska";
  if (key == "hawaii") return "Hawaii";
  if (key == "canada_atlantic") return "Canada Atlantic";
  if (key == "brazil_east") return "Brazil East";
  if (key == "argentina") return "Argentina";
  if (key == "asia_tokyo") return "Japan";
  if (key == "korea") return "South Korea";
  if (key == "australia_perth") return "Australia West";
  if (key == "australia_darwin") return "Australia Central";
  if (key == "australia_sydney") return "Australia East";
  if (key == "new_zealand") return "New Zealand";
  return "Central Europe";
}

String sanitizeTimezoneKey(const String& key) {
  const char* supported[] = {
    "utc", "atlantic_azores", "europe_west", "uk", "europe_central", "europe_east",
    "africa_south", "middle_east_gulf", "india", "thailand", "china",
    "us_eastern", "us_central", "us_mountain", "us_arizona", "us_pacific",
    "alaska", "hawaii", "canada_atlantic", "brazil_east", "argentina",
    "asia_tokyo", "korea", "australia_perth", "australia_darwin",
    "australia_sydney", "new_zealand"
  };
  for (const char* supportedKey : supported) {
    if (key == supportedKey) return key;
  }
  return "europe_central";
}

void applyDeviceTimezoneByKey(const String& key) {
  timezoneKey = sanitizeTimezoneKey(key);
  setenv("TZ", timezonePosixByKey(timezoneKey), 1);
  tzset();
}

void appendTimezoneOptions(String& page, const String& selectedKey) {
  struct TimezoneGroup {
    const char* label;
    const char* keys[8];
    int count;
  };

  const TimezoneGroup groups[] = {
    {"Global", {"utc"}, 1},
    {"Europe", {"atlantic_azores", "europe_west", "uk", "europe_central", "europe_east"}, 5},
    {"Africa & Middle East", {"africa_south", "middle_east_gulf"}, 2},
    {"Asia", {"india", "thailand", "china", "asia_tokyo", "korea"}, 5},
    {"North America", {"us_eastern", "us_central", "us_mountain", "us_arizona", "us_pacific", "alaska", "hawaii", "canada_atlantic"}, 8},
    {"South America", {"brazil_east", "argentina"}, 2},
    {"Australia & Oceania", {"australia_perth", "australia_darwin", "australia_sydney", "new_zealand"}, 4}
  };

  for (const TimezoneGroup& group : groups) {
    page += "<optgroup label='";
    page += group.label;
    page += "'>";
    for (int i = 0; i < group.count; i++) {
      const char* key = group.keys[i];
      page += "<option value='";
      page += key;
      page += "'";
      if (selectedKey == key) page += " selected";
      page += ">";
      page += timezoneLabelByKey(key);
      page += "</option>";
    }
    page += "</optgroup>";
  }
}

void getHomeSlotRect(int slot, int& x, int& y, int& w, int& h) {
  const int xs[HOME_SLOT_COUNT] = {8, 124, 8, 124, 8, 124};
  const int ys[HOME_SLOT_COUNT] = {HOME_GRID_Y1, HOME_GRID_Y1, HOME_GRID_Y2, HOME_GRID_Y2, HOME_GRID_Y3, HOME_GRID_Y3};
  x = xs[slot];
  y = ys[slot];
  w = 108;
  h = HOME_WIDGET_H;
}

void getTabSlotRect(int slot, int& x, int& y, int& w, int& h) {
  const int xs[TAB_WIDGET_COUNT] = {8, 124, 8, 124, 8, 124, 8, 124};
  const int ys[TAB_WIDGET_COUNT] = {TAB_GRID_Y1, TAB_GRID_Y1, TAB_GRID_Y2, TAB_GRID_Y2, TAB_GRID_Y3, TAB_GRID_Y3, TAB_GRID_Y4, TAB_GRID_Y4};
  x = xs[slot];
  y = ys[slot];
  w = 108;
  h = TAB_WIDGET_H;
}

static String htmlEscape(const String& s);

// Emitted once per page as a shared <template> instead of being duplicated
// into every widget-slot <select> (there are up to 30 of them: HOME_SLOT_COUNT
// plus CUSTOM_TAB_COUNT * TAB_WIDGET_COUNT). Duplicating the full option list
// per select used to add ~20-25KB of repeated HTML, which could exceed the
// page String's available heap and silently truncate the response before the
// save button and closing tags were ever appended. hydrateWidgetSelect() in
// the WebGUI script clones this template into each select client-side.
void appendHomeWidgetOptionTemplate(String& page) {
  const HomeWidgetType fixedTypes[] = {
    HOME_WIDGET_WEEK,
    HOME_WIDGET_RAIN,
    HOME_WIDGET_OUTDOOR,
    HOME_WIDGET_KP,
    HOME_WIDGET_UV,
    HOME_WIDGET_WIND,
    HOME_WIDGET_SUN
  };

  page += "<template id='widget-options'>";
  page += "<optgroup label='Widgets'>";
  for (HomeWidgetType type : fixedTypes) {
    page += "<option value='";
    page += homeWidgetKey(type);
    page += "'>";
    page += homeWidgetLabel(type);
    page += "</option>";
  }
  page += "</optgroup>";

  int visibleMqttWidgets = min(mqttValueCount, 12);
  page += "<optgroup label='MQTT Widgets'>";
  for (int i = 0; i < visibleMqttWidgets; i++) {
    HomeWidgetType type = (HomeWidgetType)((int)HOME_WIDGET_MQTT_1 + i);
    page += "<option value='";
    page += homeWidgetKey(type);
    page += "'>";
    page += homeWidgetLabel(type);
    page += "</option>";
  }
  page += "</optgroup>";
  page += "</template>";
}

void appendDataPointWidgetOptions(String& page, const String& selectedKey) {
  page += "<optgroup label='Datenpunkte'>";
  for (int i = 0; i < dataPointManager.count(); i++) {
    const DataPoint* point = dataPointManager.at(i);
    if (!point) continue;
    String key = dataPointWidgetKey(point->id);
    page += "<option value='";
    page += htmlEscape(key);
    page += "'";
    if (selectedKey == key) page += " selected";
    page += ">";
    page += htmlEscape(point->label);
    page += " (";
    page += htmlEscape(point->id);
    page += ")";
    page += "</option>";
  }
  page += "</optgroup>";
}

void appendDataPointWidgetOptionTemplate(String& page) {
  page += "<template id='dp-widget-options'><optgroup label='Datenpunkte'>";
  for (int i = 0; i < dataPointManager.count(); i++) {
    const DataPoint* point = dataPointManager.at(i);
    if (!point) continue;
    String key = dataPointWidgetKey(point->id);
    page += "<option value='";
    page += htmlEscape(key);
    page += "'>";
    page += htmlEscape(point->label);
    page += " (";
    page += htmlEscape(point->id);
    page += ")";
    page += "</option>";
  }
  page += "</optgroup></template>";
}

void clearHomeSlotCaches() {
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    cacheHomeSlots[i] = "";
  }
}

bool flashModeEnabled = false;
bool wifiEnabled = true;
bool wifiConnectInProgress = false;
unsigned long wifiConnectStartedMs = 0;
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
int wifiActiveIndex = 0;   // preferred network, selected via WebGUI
int wifiAttemptIndex = 0;  // network currently being tried (fallback rotation)
int wifiCycleStartIndex = 0;  // slot this connect cycle started from (stops the fallback rotation)

// Runtime Wi-Fi credentials (1.0.5). Seeded from the compile-time WIFI_NETWORKS
// on first boot (see loadStoredSettings()), then editable in the WebGUI and
// persisted in NVS. Empty-SSID slots are skipped by the connection logic.
String wifiSlotSsid[WIFI_SLOT_COUNT];
String wifiSlotPass[WIFI_SLOT_COUNT];

// SoftAP onboarding portal (1.0.6) - see startSetupPortal() in WifiRuntime.inc.
// Active when the device could not join any configured network at boot (or was
// started manually from the WebGUI); serves a captive setup page over its own
// access point.
DNSServer dnsServer;
bool portalActive = false;
String portalApSsid;
// While the portal is active, STA connect attempts drag the shared radio
// across channels and the SoftAP beacon follows - phones then cannot see or
// hold the setup network. The reconnect scheduler is therefore throttled to
// this slow cadence while portalActive (1.0.7); /setup/connect still triggers
// an immediate attempt via beginWiFiConnect().
unsigned long portalLastAttemptMs = 0;
const unsigned long PORTAL_RECONNECT_INTERVAL_MS = 90000UL;

// Deferred WebGUI-requested restart (1.0.7): handleRestart() only sets the
// flag; loop() reboots after a short grace period so the HTTP "OK" response
// reaches the browser before the connection dies.
bool restartPending = false;
unsigned long restartPendingAtMs = 0;
const unsigned long RESTART_GRACE_MS = 750UL;

// WebGUI login (1.0.7): runtime credentials, stored in NVS and editable in
// the settings tab. Seeded from the compile-time WEBGUI_USER/WEBGUI_PASS on
// first boot, so the public build's documented default (admin/infoterm) only
// applies until the user changes it. An empty password disables the login
// gate (see webAuthOk() in WebGuiRuntime.inc). Never part of a backup export.
String webUser = WEBGUI_USER;
String webPass = WEBGUI_PASS;

// IP configuration (0.9.31). "dhcp" (default) or "static". The static fields
// are applied via WiFi.config() before WiFi.begin(); see applyIpConfiguration()
// in WifiRuntime.inc. Persisted in NVS and part of Backup/Restore.
String ipMode = INFOTERM_IP_MODE;
String staticIp = INFOTERM_STATIC_IP;
String staticGateway = INFOTERM_STATIC_GATEWAY;
String staticSubnet = INFOTERM_STATIC_SUBNET;
String staticDns = INFOTERM_STATIC_DNS;

// Background WiFi scan results (see schedulerWifiScan() in Application.inc):
// which configured WIFI_NETWORKS[] entries were seen in the most recent scan,
// shown as a hint in the WebGUI's network dropdown. Runtime-only, never
// persisted.
bool wifiNetworkReachable[WIFI_SLOT_COUNT] = {};
bool wifiScanEverCompleted = false;

// VPN (WireGuard, on-demand) - see src/vpn/VpnRuntime.inc. vpnConnected is
// deliberately runtime-only (never persisted) so a reboot always starts
// disconnected, never auto-resuming into a state that could strand the
// device - see feedback-esp32-wireguard-breaks-lan.md.
bool vpnConnected = false;
String vpnStatusMessage = "Getrennt";
unsigned long vpnConnectedSinceMs = 0;
String vpnPrivateKey = "";
String vpnPeerPublicKey = "";
String vpnEndpointHost = "";
uint16_t vpnEndpointPort = 51820;
String vpnLocalAddress = "";
String vpnDns = "";
String vpnAllowedIps = "";
String vpnName = "";

// Wetter
static float tempC = NAN;
static float tempMinC = NAN;
static float tempMaxC = NAN;
static float precipMm = NAN;
static float windSpeedMs = NAN;
static float windDirectionDeg = NAN;
static float uvIndex = NAN;
static float humidityPct = NAN;
static float pressureHPa = NAN;
static time_t lastWeatherFetch = 0;
static time_t lastMqttWeatherFetch = 0;
static const uint32_t WEATHER_INTERVAL_SEC = 10 * 60;
static const uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
static unsigned long lastMqttReconnectAttemptMs = 0;

// KP-index
static float kpIndex = NAN;
static time_t lastKpFetch = 0;
static const uint32_t KP_INTERVAL_SEC = 10 * 60;

// Sunrise / Sunset
static int sunriseMin = -1;
static int sunsetMin  = -1;
static int lastSunYmd = -1;
static time_t lastSyncTime = 0;

// =========================================================
// SLEEP / BACKLIGHT
// =========================================================
const int BACKLIGHT_PIN = 21;

bool sleepDimmed = false;
bool sleepOff = false;
bool manualDimMode = false;

unsigned long lastInteractionMs = 0;

int sleepIntervalMin = 10;
int sleepOffDelaySec = 60;

// Tab rotation / burn-in guard (1.0.2). When auto-sleep is "Never"
// (sleepIntervalMin == 0) and this is enabled, the visible tabs (all except
// the Info page) are cycled every tabCycleIntervalSec seconds so no single
// screen stays fixed. Persisted and part of Backup/Restore.
bool tabCycleEnabled = false;
int tabCycleIntervalSec = 60;   // one of 30 / 60 / 180 / 360
unsigned long lastTabCycleMs = 0;

int logLevelValue = static_cast<int>(Logger::Level::Info);

const int BL_FULL = 255;
const int BL_DIM  = 18;
const int BL_OFF  = 0;
const int FLASH_BL_LOW = 20;
const int FLASH_BL_HIGH = 255;

// Awake-state backlight level: while the night palette is active the panel
// is dimmed to the persisted percentage, otherwise full brightness. Sleep
// dim/off (BL_DIM/BL_OFF) are separate states and continue to apply on top.
int awakeBacklightLevel() {
  if (!nightModeActive) return BL_FULL;
  return constrain((nightBrightnessPct * BL_FULL) / 100, BL_DIM, BL_FULL);
}


// Forward declarations needed by Arduino IDE / ESP32 core 3.x
void setBacklight(int value);
void setWifiEnabled(bool enabled);

void wakeDisplay(bool clearManualMode = true);


// =========================================================
// HELPERS
// =========================================================
static int ymdFromLocal(time_t t) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  return (tmLocal.tm_year + 1900) * 10000 + (tmLocal.tm_mon + 1) * 100 + tmLocal.tm_mday;
}

static int minutesFromLocalEpoch(time_t t) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  return tmLocal.tm_hour * 60 + tmLocal.tm_min;
}

static int minutesNowLocal() {
  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  return tmNow.tm_hour * 60 + tmNow.tm_min;
}

static String wifiStatusText() {
  if (!wifiEnabled) return String(TXT(L_STATUS_DISABLED));
  return WiFi.status() == WL_CONNECTED ? String(TXT(L_STATUS_ONLINE)) : String(TXT(L_STATUS_OFFLINE));
}

static String signalText() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED) return "-- dBm";
  return String(WiFi.RSSI()) + " dBm";
}

static String ipText() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED) return "-";
  return WiFi.localIP().toString();
}

static bool useUsRegionFormat() {
  return regionFormatKey == "us";
}

static String formatClockParts(const struct tm& tmValue, bool withSeconds) {
  char buf[20];
  const char* pattern = useUsRegionFormat()
    ? (withSeconds ? "%I:%M:%S %p" : "%I:%M %p")
    : (withSeconds ? "%H:%M:%S" : "%H:%M");
  strftime(buf, sizeof(buf), pattern, &tmValue);
  return String(buf);
}

static String formatDateParts(const struct tm& tmValue) {
  char buf[32];
  if (useUsRegionFormat()) {
    snprintf(buf, sizeof(buf), "%s %02d/%02d/%04d",
             localizedWeekdayShort(tmValue.tm_wday),
             tmValue.tm_mon + 1,
             tmValue.tm_mday,
             tmValue.tm_year + 1900);
  } else {
    snprintf(buf, sizeof(buf), "%s %02d.%02d.%04d",
             localizedWeekdayShort(tmValue.tm_wday),
             tmValue.tm_mday,
             tmValue.tm_mon + 1,
             tmValue.tm_year + 1900);
  }
  return String(buf);
}

static String formatMinuteOfDay(int minOfDay) {
  if (minOfDay < 0) return "--:--";
  if (useUsRegionFormat()) {
    int hour24 = minOfDay / 60;
    int minute = minOfDay % 60;
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;
    char buf[12];
    snprintf(buf, sizeof(buf), "%d:%02d %s", hour12, minute, hour24 >= 12 ? "PM" : "AM");
    return String(buf);
  }
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", minOfDay / 60, minOfDay % 60);
  return String(buf);
}

static String tempText() {
  if (isnan(tempC)) return unitKey == "imperial" ? "--.-F" : "--.-C";

  if (unitKey == "imperial") {
    float f = tempC * 9.0f / 5.0f + 32.0f;
    return String(f, 1) + "F";
  }

  return String(tempC, 1) + "C";
}

static String formatDisplayTemp(float value) {
  if (isnan(value)) return "--";

  if (unitKey == "imperial") {
    float f = value * 9.0f / 5.0f + 32.0f;
    return String((int)roundf(f)) + "F";
  }

  return String((int)roundf(value)) + "C";
}

static String tempRangeText() {
  return "H:" + formatDisplayTemp(tempMaxC) + "  L:" + formatDisplayTemp(tempMinC);
}

static String rainText() {
  if (isnan(precipMm)) return unitKey == "imperial" ? "--.--in" : "--.-mm";

  if (unitKey == "imperial") {
    float inches = precipMm / 25.4f;
    return String(inches, 2) + "in";
  }

  return String(precipMm, 1) + "mm";
}

static String windText() {
  if (isnan(windSpeedMs)) return unitKey == "imperial" ? "--.-mph" : "--.-m/s";

  if (unitKey == "imperial") {
    float mph = windSpeedMs * 2.236936f;
    return String(mph, 1) + "mph";
  }

  return String(windSpeedMs, 1) + "m/s";
}

static String windDirectionText() {
  if (isnan(windDirectionDeg)) return "--";

  const char* dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int idx = (int)roundf(windDirectionDeg / 45.0f) % 8;
  return String(dirs[idx]) + " " + String((int)roundf(windDirectionDeg)) + "deg";
}

static String kpText() {
  return isnan(kpIndex) ? "Kp --" : "Kp " + String(kpIndex, 1);
}

static String kpLevelText() {
  if (isnan(kpIndex)) return "--";
  if (kpIndex < 3.0f) return String(TXT(L_LEVEL_LOW));
  if (kpIndex < 5.0f) return String(TXT(L_LEVEL_MEDIUM));
  if (kpIndex < 7.0f) return String(TXT(L_LEVEL_HIGH));
  return String(TXT(L_LEVEL_EXTREME));
}

static String uvText() {
  return isnan(uvIndex) ? "UV --" : "UV " + String(uvIndex, 1);
}

static String uvLevelText() {
  if (isnan(uvIndex)) return "--";
  if (uvIndex < 3.0f) return String(TXT(L_LEVEL_LOW));
  if (uvIndex < 6.0f) return String(TXT(L_LEVEL_MODERATE));
  if (uvIndex < 8.0f) return String(TXT(L_LEVEL_HIGH));
  if (uvIndex < 11.0f) return String(TXT(L_LEVEL_VERY_HIGH));
  return String(TXT(L_LEVEL_EXTREME));
}

static uint16_t statusColor() {
  if (textColorKey != "standard") return COL_TEXT;
  if (!wifiEnabled) return COL_YELLOW;
  return WiFi.status() == WL_CONNECTED ? COL_GREEN : COL_RED;
}

static String uptimeText() {
  unsigned long seconds = millis() / 1000UL;
  unsigned long days = seconds / 86400UL;
  seconds %= 86400UL;
  unsigned long hours = seconds / 3600UL;
  seconds %= 3600UL;
  unsigned long minutes = seconds / 60UL;

  if (days > 0) return String(days) + "d " + String(hours) + "h";
  if (hours > 0) return String(hours) + "h " + String(minutes) + "m";
  return String(minutes) + "m";
}

static String nextSunLabel() {
  int nowMin = minutesNowLocal();
  if (sunriseMin < 0 || sunsetMin < 0) return TXT(L_SUN);
  if (nowMin < sunriseMin) return TXT(L_SUNRISE);
  if (nowMin < sunsetMin) return TXT(L_SUNSET);
  return TXT(L_SUNRISE);
}

static String nextSunTimeText() {
  int nowMin = minutesNowLocal();
  if (sunriseMin < 0 || sunsetMin < 0) return "--:--";
  if (nowMin < sunriseMin) return formatMinuteOfDay(sunriseMin);
  if (nowMin < sunsetMin) return formatMinuteOfDay(sunsetMin);
  return formatMinuteOfDay(sunriseMin);
}

// Thin wrapper over the host-tested infoterm::htmlEscaped (InfoTermLogic.h);
// covers &, <, >, " AND ' - the WebGUI uses single-quoted attributes.
static String htmlEscape(const String& s) {
  return infoterm::htmlEscaped<String>(s.c_str());
}

static bool parseDisplayNumber(const String& input, float& outValue) {
  String s = input;
  s.trim();
  s.replace(",", ".");
  if (s.length() == 0) return false;

  char* endPtr = nullptr;
  float value = strtof(s.c_str(), &endPtr);
  if (endPtr == s.c_str()) return false;

  while (endPtr && *endPtr != '\0') {
    if (!isspace((unsigned char)*endPtr)) return false;
    endPtr++;
  }

  outValue = value;
  return true;
}

static String mqttFormattedValue(int idx) {
  if (idx < 0 || idx >= MQTT_VALUE_COUNT) return "--";
  String value = mqttValues[idx];
  value.trim();
  if (value.length() == 0) return "--";

  int decimals = mqttDecimals[idx];
  if (decimals == MQTT_DECIMALS_AUTO) return value;
  decimals = constrain(decimals, 0, 4);

  float numericValue = NAN;
  if (!parseDisplayNumber(value, numericValue)) return value;
  return String(numericValue, (unsigned int)decimals);
}

// Runtime functions moved to the DataPoint module.
#include "datapoints/DataPointRuntime.inc"

static String cssColorFrom565(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
  return String(buf);
}

// Single shared color palette used by every color picker on the Design tab
// (accent, border, text, menu/button/widget background AND the page
// background theme). Background theme tones (COL_BG/COL_PANEL/PANEL_ALT/
// STROKE) are derived automatically from each entry's base color via
// mixRgb565() rather than hand-picked per theme, so all pickers stay in
// sync with a single source of truth.
struct PaletteEntry { const char* key; const char* label[LANG_COUNT]; uint16_t rgb565; };  // label order: de, en, fr, es, it

// 0.9.24: extended back to the larger pre-consolidation range (user request,
// reconstructed from a screenshot of the earlier WebGUI - the 0.9.12-0.9.15
// sources no longer exist anywhere). All previous keys are kept so stored
// configurations keep resolving; "blue" additionally restores the pre-
// 0.9.11.10 accent key of the same name. Order: light -> colors -> cyan ->
// dark, matching the historical row layout.
static const PaletteEntry COLOR_PALETTE[] = {
  {"white",      {"Weiß", "White", "Blanc", "Blanco", "Bianco", "Белый", "सफ़ेद", "白色"},                        TFT_WHITE},
  {"ice",        {"Eis", "Ice", "Glace", "Hielo", "Ghiaccio", "Лёд", "बर्फ़", "冰蓝"},                          0xEFFF},
  {"silver",     {"Silber", "Silver", "Argent", "Plata", "Argento", "Серебристый", "चाँदी", "银色"},            0xC618},
  {"mint",       {"Minze", "Mint", "Menthe", "Menta", "Menta", "Мятный", "पुदीना", "薄荷绿"},                    0x07F0},
  {"green",      {"Grün", "Green", "Vert", "Verde", "Verde", "Зелёный", "हरा", "绿色"},                          TFT_GREEN},
  {"teal",       {"Türkis", "Teal", "Sarcelle", "Verde azulado", "Verde acqua", "Бирюзовый", "टील", "青色"},    0x2EB7},
  {"blue",       {"Blau", "Blue", "Bleu", "Azul", "Blu", "Синий", "नीला", "蓝色"},                              0x3D9F},
  {"purple",     {"Violett", "Purple", "Violet", "Púrpura", "Viola", "Фиолетовый", "बैंगनी", "紫色"},           0xA2F5},
  {"lavender",   {"Flieder", "Lavender", "Lavande", "Lavanda", "Lavanda", "Лавандовый", "लैवेंडर", "薰衣草紫"},  0xB41F},
  {"pink",       {"Magenta", "Magenta", "Magenta", "Magenta", "Magenta", "Пурпурный", "मैजेंटा", "品红"},       0xF97F},
  {"orange",     {"Orange", "Orange", "Orange", "Naranja", "Arancione", "Оранжевый", "नारंगी", "橙色"},         0xFD20},
  {"amber",      {"Gold", "Gold", "Or", "Oro", "Oro", "Золотой", "सुनहरा", "金色"},                             0xFEA0},
  {"yellow",     {"Gelb", "Yellow", "Jaune", "Amarillo", "Giallo", "Жёлтый", "पीला", "黄色"},                   0xFFE0},
  {"red",        {"Rot", "Red", "Rouge", "Rojo", "Rosso", "Красный", "लाल", "红色"},                            TFT_RED},
  {"cyan",       {"Cyan", "Cyan", "Cyan", "Cian", "Ciano", "Голубой", "सियान", "青蓝"},                         0x5EFA},
  {"black",      {"Schwarz", "Black", "Noir", "Negro", "Nero", "Чёрный", "काला", "黑色"},                       0x0862},
  {"anthracite", {"Anthrazit", "Anthracite", "Anthracite", "Antracita", "Antracite", "Антрацит", "एन्थ्रेसाइट", "炭灰"}, 0x2966},
  {"graphite",   {"Graphit", "Graphite", "Graphite", "Grafito", "Grafite", "Графит", "ग्रेफ़ाइट", "石墨灰"},     0x4228},
  {"petrol",     {"Dunkelpetrol", "Dark petrol", "Pétrole foncé", "Petróleo oscuro", "Petrolio scuro", "Тёмный петроль", "गहरा पेट्रोल", "深孔雀蓝"}, 0x1269},
  {"navy",       {"Tiefblau", "Navy", "Bleu marine", "Azul marino", "Blu navy", "Тёмно-синий", "नेवी", "藏青"},  0x19CB},
  {"indigo",     {"Indigo", "Indigo", "Indigo", "Índigo", "Indaco", "Индиго", "इंडिगो", "靛蓝"},                0x18C9},
  {"olive",      {"Oliv", "Olive", "Olive", "Oliva", "Oliva", "Оливковый", "जैतून", "橄榄绿"},                  0x5AA3},
};
static inline const char* paletteLabel(const PaletteEntry& e) {
  return e.label[clampLanguageIndex()];
}
static const int COLOR_PALETTE_COUNT = sizeof(COLOR_PALETTE) / sizeof(COLOR_PALETTE[0]);

static const PaletteEntry* findPaletteEntry(const String& key) {
  for (int i = 0; i < COLOR_PALETTE_COUNT; i++) {
    if (key == COLOR_PALETTE[i].key) return &COLOR_PALETTE[i];
  }
  return nullptr;
}

// Blends a 565 color component-wise toward (towardR,towardG,towardB) by
// factor t (0 = unchanged, 1 = fully the target color). Used to derive
// darker/lighter tones from a single base hue.
// Thin wrapper over the host-tested infoterm::mixRgb565 (include/InfoTermLogic.h)
// so the many call sites keep the short name.
static uint16_t mixRgb565(uint16_t base565, uint8_t towardR, uint8_t towardG, uint8_t towardB, float t) {
  return infoterm::mixRgb565(base565, towardR, towardG, towardB, t);
}

static String accentPreviewCss(const String& key) {
  if (key == "standard") return cssColorFrom565(0xEF7D);
  const PaletteEntry* e = findPaletteEntry(key);
  return cssColorFrom565(e ? e->rgb565 : 0xEF7D);
}

static String borderPreviewCss(const String& key) {
  return accentPreviewCss(key);
}

static uint16_t color565FromPaletteKey(const String& key, uint16_t fallback) {
  if (key == "standard") return 0xEF7D;
  const PaletteEntry* e = findPaletteEntry(key);
  return e ? e->rgb565 : fallback;
}

static String lastSyncText() {
  if (lastSyncTime <= 0) return String(TXT(L_STATUS_SYNC)) + " --:--";

  struct tm tmSync;
  localtime_r(&lastSyncTime, &tmSync);
  return String(TXT(L_STATUS_SYNC)) + " " + formatClockParts(tmSync, false);
}

static String lastSyncValueText() {
  if (lastSyncTime <= 0) return "--:--";

  struct tm tmSync;
  localtime_r(&lastSyncTime, &tmSync);
  return formatClockParts(tmSync, false);
}

static String weekNumberText() {
  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  char buf[4];
  strftime(buf, sizeof(buf), "%V", &tmNow);
  return String(buf);
}


static String homeTitleText() {
  return deviceName.length() > 0 ? deviceName : "InfoTerm";
}


// Runtime functions moved to the System module.
#include "system/SystemRuntime.inc"

// =========================================================
// THEME / SETTINGS
// =========================================================
void applyThemeByKey(const String& accentKey, const String& bgKey) {
  accentColorKey = accentKey;
  backgroundColorKey = bgKey;
  COL_ACCENT = color565FromPaletteKey(accentKey, 0x5EFA);

  // Background theme tones are derived from a single palette base color
  // instead of ten hand-picked 4-tone sets, so bg shares the exact same
  // palette as accent/border/text/menu/button/widget background. The mix
  // factors are deliberately mild (not >0.8) so a derived panel tone stays
  // visibly tinted by its base hue instead of crushing down to near-black
  // - otherwise every theme's panels look identical to the "Schwarz"
  // swatch regardless of which base color was picked.
  const PaletteEntry* bgEntry = findPaletteEntry(bgKey);
  uint16_t bgBase = bgEntry ? bgEntry->rgb565 : 0x18C9;
  COL_BG        = mixRgb565(bgBase, 0, 0, 0, 0.80f);
  COL_PANEL     = mixRgb565(bgBase, 0, 0, 0, 0.65f);
  COL_PANEL_ALT = mixRgb565(bgBase, 0, 0, 0, 0.55f);
  COL_STROKE    = mixRgb565(bgBase, 180, 180, 180, 0.5f);
}

void applyBorderColorByKey(const String& key) {
  borderColorKey = key;
  if (key == "theme" || key.length() == 0) borderColorKey = "cyan";
  COL_STROKE = color565FromPaletteKey(borderColorKey, COL_STROKE);
}

void applyAreaColorByKey(String& storedKey, uint16_t& targetColor, const String& key, uint16_t themeColor) {
  storedKey = key;
  if (storedKey.length() == 0 || storedKey == "theme") {
    storedKey = "theme";
    targetColor = themeColor;
  } else {
    targetColor = color565FromPaletteKey(storedKey, themeColor);
  }
}

void applyMenuBgColorByKey(const String& key) {
  applyAreaColorByKey(menuBgColorKey, COL_MENU_BG, key, COL_PANEL_ALT);
}

void applyButtonBgColorByKey(const String& key) {
  applyAreaColorByKey(buttonBgColorKey, COL_BUTTON_BG, key, COL_PANEL);
}

void applyWidgetBgColorByKey(const String& key) {
  applyAreaColorByKey(widgetBgColorKey, COL_PANEL, key, COL_PANEL);
}

void applyTextColorByKey(const String& key) {
  textColorKey = key;

  if (key == "standard") {
    COL_TEXT = 0xEF7D; COL_DIM = 0x94B2;
    return;
  }

  const PaletteEntry* e = findPaletteEntry(key);
  if (!e) {
    COL_TEXT = 0xEF7D;
    COL_DIM  = 0x94B2;
    textColorKey = "standard";
    return;
  }

  COL_TEXT = e->rgb565;
  COL_DIM = mixRgb565(e->rgb565, 0, 0, 0, 0.35f);
}

// Re-derives the normal ("day") theme from the persisted *ColorKey strings -
// this is the exact same set of calls used at boot/on save, bundled so
// night mode can cleanly restore the user's actual chosen theme without
// needing a separate saved snapshot.
void applyStoredDesignColors() {
  applyThemeByKey(accentColorKey, backgroundColorKey);
  applyMenuBgColorByKey(menuBgColorKey);
  applyButtonBgColorByKey(buttonBgColorKey);
  applyWidgetBgColorByKey(widgetBgColorKey);
  applyBorderColorByKey(borderColorKey);
  applyTextColorByKey(textColorKey);
}

// Fixed "night vision" red palette (see docs/ROADMAP.md "Nachtmodus" - the
// user chose a fixed palette over one derived from the accent color, so
// the result is predictable regardless of which theme is otherwise
// selected). Deliberately does not touch the persisted *ColorKey strings -
// applyStoredDesignColors() remains the way back to the day theme.
// COL_GREEN/COL_RED/COL_YELLOW/COL_BLUE (status indicators like "MQTT
// connected") are deliberately left untouched - night mode re-themes the
// Design-tab colors, not the semantic status colors.
void applyNightModeColors() {
  // All backgrounds pure black (user feedback from the first live test: no
  // red-tinted fills behind widgets, menu or inactive buttons) - red is
  // reserved for text, accents and strokes.
  // Base tone since 0.9.29 (user feedback): a darker crimson rgb(176,0,32)
  // instead of pure TFT_RED - full-brightness red rendered visibly
  // orange-ish on this ST7789 panel; the lower brightness plus a touch of
  // blue removes the orange cast.
  // 1.0.8 (user request): text considerably darker - blended toward black
  // instead of the former 15% lightening; COL_DIM pushed further down so
  // primary and secondary text stay distinguishable. On top of that the
  // backlight itself is dimmed at night (see awakeBacklightLevel()).
  const uint16_t base = 0xB004;
  COL_BG        = TFT_BLACK;
  COL_PANEL     = TFT_BLACK;
  COL_PANEL_ALT = TFT_BLACK;
  COL_MENU_BG   = TFT_BLACK;
  COL_BUTTON_BG = TFT_BLACK;
  COL_STROKE    = mixRgb565(base, 0, 0, 0, 0.35f);
  COL_ACCENT    = mixRgb565(base, 0, 0, 0, 0.10f);
  COL_TEXT      = mixRgb565(base, 0, 0, 0, 0.35f);
  COL_DIM       = mixRgb565(base, 0, 0, 0, 0.60f);
}

// sunriseMin/sunsetMin are -1 until the first successful weather fetch -
// treated as "daytime" until then, so night mode simply activates a little
// later after boot rather than guessing.
bool isNightNow() {
  return infoterm::isNightWindow(minutesNowLocal(), sunriseMin, sunsetMin);
}

// Single decision point for the night overlay: forced mode wins regardless
// of the sun window (and independent of the auto checkbox, so daytime
// testing needs only the one switch). Used by boot load, WebGUI save and
// the periodic transition check alike.
bool shouldNightBeActive() {
  return nightModeForced || (nightModeEnabled && isNightNow());
}

// Periodic check (called from schedulerWeatherUpdate(), see Application.inc)
// for the sunset/sunrise transition. Only acts on an actual state change -
// toggling the feature checkbox itself and reasserting the night palette
// after a WebGUI save are handled directly in handleSave()/loadStoredSettings().
void checkNightModeTransition() {
  bool shouldBeNight = shouldNightBeActive();
  if (shouldBeNight == nightModeActive) return;
  nightModeActive = shouldBeNight;
  if (nightModeActive) applyNightModeColors();
  else applyStoredDesignColors();
  // Follow the transition with the matching panel brightness, but never
  // override an active sleep dim/off state - those wake up through
  // wakeDisplay(), which asks awakeBacklightLevel() itself.
  if (!sleepDimmed && !sleepOff) setBacklight(awakeBacklightLevel());
  pageDirty = true;
}


void persistRuntimeConfiguration();
#include "storage/StorageLoadRuntime.inc"


void resetDataCaches() {
  tempC = NAN;
  precipMm = NAN;
  windSpeedMs = NAN;
  windDirectionDeg = NAN;
  kpIndex = NAN;
  sunriseMin = -1;
  sunsetMin = -1;
  lastSunYmd = -1;
  lastWeatherFetch = 0;
  lastKpFetch = 0;
  dataDirty = true;
  pageDirty = true;
}

// =========================================================
// TOUCH
// =========================================================
bool readTouchXY(int& sx, int& sy) {
  if (!ts.touched()) return false;

  TS_Point p = ts.getPoint();
  if (p.z < 80 || p.z > 4000) return false;

  int x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W);
  int y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H);

  x = constrain(x, 0, SCREEN_W - 1);
  y = constrain(y, 0, SCREEN_H - 1);

  if (TOUCH_SWAP_XY) { int tmp = x; x = y; y = tmp; }
  if (TOUCH_FLIP_X)  x = (SCREEN_W - 1) - x;
  if (TOUCH_FLIP_Y)  y = (SCREEN_H - 1) - y;

  // 180-degree display flip: the touch panel stays on the calibrated base
  // orientation, so both mapped screen axes get mirrored here instead.
  if (displayRotated180) {
    x = (SCREEN_W - 1) - x;
    y = (SCREEN_H - 1) - y;
  }

  sx = x;
  sy = y;
  return true;
}

bool touchNewPress(int& tx, int& ty) {
  static bool wasDown = false;
  static unsigned long lastPressMs = 0;

  bool down = false;
  int x = 0, y = 0;

  if (readTouchXY(x, y)) down = true;

  bool fire = false;
  unsigned long now = millis();

  if (down && !wasDown && (now - lastPressMs > 220)) {
    fire = true;
    lastPressMs = now;
    tx = x;
    ty = y;
  }

  wasDown = down;
  return fire;
}


// =========================================================
// MQTT / HOME ASSISTANT
// =========================================================
#include "mqtt/MqttRuntime.inc"

// =========================================================
// API
// =========================================================
#include "weather/WeatherRuntime.inc"

// =========================================================
// DRAW HELPERS
// =========================================================
#include "display/DisplayRuntime.inc"

Page pageForCustomTab(int tabIndex) {
  switch (tabIndex) {
    case 0: return PAGE_TAB1;
    case 1: return PAGE_TAB2;
    default: return PAGE_TAB3;
  }
}

int customTabIndexFromPage(Page page) {
  if (page == PAGE_TAB1) return 0;
  if (page == PAGE_TAB2) return 1;
  if (page == PAGE_TAB3) return 2;
  return -1;
}

int visibleNavPages(Page pages[], String labels[], int maxPages) {
  int count = 0;
  if (count < maxPages) { pages[count] = PAGE_HOME; labels[count] = String(TXT(L_TAB_START)); count++; }
  for (int i = 0; i < CUSTOM_TAB_COUNT; i++) {
    if (!customTabEnabled[i]) continue;
    if (count < maxPages) { pages[count] = pageForCustomTab(i); labels[count] = customTabNames[i]; count++; }
  }
  if (count < maxPages) { pages[count] = PAGE_INFO; labels[count] = String(TXT(L_TAB_STATUS)); count++; }
  return count;
}

// Next visible tab after `current`, skipping the Info page (1.0.2 tab
// rotation). If `current` is the Info page (user opened it manually), the
// rotation resumes at the first non-Info tab. Returns `current` unchanged
// when there are fewer than two rotatable tabs (nothing to cycle).
Page nextCyclePage(Page current) {
  Page pages[5];
  String labels[5];
  int count = visibleNavPages(pages, labels, 5);

  Page cyclePages[5];
  int cycleCount = 0;
  for (int i = 0; i < count; i++) {
    if (pages[i] != PAGE_INFO) cyclePages[cycleCount++] = pages[i];
  }
  if (cycleCount < 2) return current;

  int idx = -1;
  for (int i = 0; i < cycleCount; i++) {
    if (cyclePages[i] == current) { idx = i; break; }
  }
  return cyclePages[(idx + 1) % cycleCount];
}

// Advances to the next tab once tabCycleIntervalSec has elapsed. Only runs
// when the burn-in rotation is enabled AND auto-sleep is "Never" - if a sleep
// timeout is set, that already prevents a static burned-in image. Called from
// schedulerSystemStatus() (5 s cadence; a few seconds of jitter on a 30 s+
// interval is irrelevant). Manual touch and a WebGUI save reset lastTabCycleMs
// so the user always gets a full interval on the tab they just selected.
void handleTabCycle() {
  if (!tabCycleEnabled || sleepIntervalMin > 0) return;
  if (sleepDimmed || sleepOff) return;  // don't rotate a dimmed/off screen
  unsigned long now = millis();
  if (now - lastTabCycleMs < (unsigned long)tabCycleIntervalSec * 1000UL) return;
  lastTabCycleMs = now;
  Page next = nextCyclePage(currentPage);
  if (next != currentPage) {
    currentPage = next;
    pageDirty = true;
  }
}

// Small gear glyph for the Info/settings nav button - replaces the "Info"
// text label (user request 0.9.29). Drawn from primitives since no icon
// font is compiled in: 8 stubby teeth, filled body, punched-out hub.
static void drawGearIcon(int cx, int cy, uint16_t color, uint16_t bg) {
  const int teeth = 8;
  for (int i = 0; i < teeth; i++) {
    float a = (2.0f * PI * i) / teeth;
    int x0 = cx + (int)roundf(cosf(a) * 5.0f);
    int y0 = cy + (int)roundf(sinf(a) * 5.0f);
    int x1 = cx + (int)roundf(cosf(a) * 9.0f);
    int y1 = cy + (int)roundf(sinf(a) * 9.0f);
    tft.drawLine(x0, y0, x1, y1, color);
    tft.drawLine(x0 + 1, y0, x1 + 1, y1, color);
    tft.drawLine(x0, y0 + 1, x1, y1 + 1, color);
  }
  tft.fillCircle(cx, cy, 6, color);
  tft.fillCircle(cx, cy, 3, bg);
}

void drawNavBar() {
  const int y = SCREEN_H - NAV_H;
  tft.fillRect(0, y, SCREEN_W, NAV_H, COL_MENU_BG);

  Page pages[5];
  String labels[5];
  int count = visibleNavPages(pages, labels, 5);
  if (count <= 0) return;
  const int btnW = SCREEN_W / count;

  for (int i = 0; i < count; i++) {
    int bx = i * btnW;
    bool active = (currentPage == pages[i]);

    uint16_t bg = active ? COL_ACCENT : COL_BUTTON_BG;
    uint16_t fg = active ? TFT_BLACK : COL_TEXT;
    if (nightModeActive && active) {
      // Night mode: no filled surfaces at all - the active button is marked
      // by its red outline and red label on black instead of a red fill.
      bg = COL_BUTTON_BG;
      fg = COL_ACCENT;
    }

    tft.fillRoundRect(bx + 3, y + 6, btnW - 6, NAV_H - 12, 8, bg);
    tft.drawRoundRect(bx + 3, y + 6, btnW - 6, NAV_H - 12, 8, active ? COL_ACCENT : COL_BUTTON_BG);

    if (pages[i] == PAGE_INFO) {
      drawGearIcon(bx + btnW / 2, y + NAV_H / 2, fg, bg);
    } else {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(fg, bg);
      tft.drawString(labels[i], bx + btnW / 2, y + NAV_H / 2, 1);
    }
  }

  tft.setTextDatum(TL_DATUM);
}

#include "widgets/WidgetRuntime.inc"
#include "vpn/VpnRuntime.inc"

// =========================================================
// PAGES
// =========================================================
void drawHomePageFull() {
  tft.fillScreen(COL_BG);
  drawTopBar(homeTitleText());
  drawNavBar();

  cacheClock = "";
  cacheHomeEmpty1 = "";
  cacheHomeEmpty2 = "";
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    cacheHomeSlots[i] = "";
  }

  pageDirty = false;
  lastDrawnPage = PAGE_HOME;

  drawClockCardSprite(true);
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    drawHomeSlotWidget(i, true);
  }
}

void updateHomeDynamic() {

  drawClockCardSprite(false);
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    drawHomeSlotWidget(i, false);
  }
}

void drawCustomTabPageFull(int tabIndex) {
  tft.fillScreen(COL_BG);
  drawNavBar();

  pageDirty = false;
  lastDrawnPage = pageForCustomTab(tabIndex);

  for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
    cacheCustomTabSlots[tabIndex][i] = "";
  }

  for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
    drawCustomTabWidget(tabIndex, i, true);
  }
}

void updateCustomTabDynamic(int tabIndex) {
  for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
    drawCustomTabWidget(tabIndex, i, false);
  }
}

void drawWeatherPageFull() {
  tft.fillScreen(COL_BG);
  drawTopBar(TXT(L_TAB_WEATHER));
  drawNavBar();

  drawCard(8, PAGE_ROW1_Y, 108, PAGE_WIDGET_H, true);
  drawCard(124, PAGE_ROW1_Y, 108, PAGE_WIDGET_H, true);
  drawCard(8, PAGE_ROW2_Y, 108, PAGE_WIDGET_H, true);
  drawCard(124, PAGE_ROW2_Y, 108, PAGE_WIDGET_H, true);
  drawCard(8, PAGE_ROW3_Y, 108, PAGE_WIDGET_H, true);
  drawCard(124, PAGE_ROW3_Y, 108, PAGE_WIDGET_H, true);

  pageDirty = false;
  lastDrawnPage = PAGE_WEATHER;

  lastTempText = "";
  lastRainText = "";
  lastUvText = "";
  lastUvLevelText = "";
  lastKpText = "";
  lastKpLevelText = "";
  lastWindText = "";
  lastWindDirText = "";
  lastNextSunLabel = "";
  lastNextSunTime = "";
}

void updateWeatherDynamic() {
  String t = tempText();
  if (t != lastTempText) {
    tft.fillRect(18, PAGE_ROW1_Y + 30, 88, 30, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(TXT(L_WEATHER_OUTDOOR), 18, PAGE_ROW1_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    tft.drawString(t, 18, PAGE_ROW1_Y + 30, 4);
    lastTempText = t;
  }

  String r = rainText();
  if (r != lastRainText) {
    tft.fillRect(134, PAGE_ROW1_Y + 30, 88, 24, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(TXT(L_WEATHER_RAIN), 134, PAGE_ROW1_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    tft.drawString(r, 134, PAGE_ROW1_Y + 30, 4);
    lastRainText = r;
  }

  String u = uvText();
  if (u != lastUvText || dataDirty) {
    tft.fillRect(18, PAGE_ROW2_Y + 30, 88, 30, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(TXT(L_WEATHER_UV), 18, PAGE_ROW2_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    tft.drawString(u, 18, PAGE_ROW2_Y + 28, 4);
    lastUvText = u;
  }

  String w = windText();
  if (w != lastWindText) {
    tft.fillRect(134, PAGE_ROW2_Y + 30, 88, 30, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(TXT(L_WEATHER_WIND), 134, PAGE_ROW2_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    tft.drawString(w, 134, PAGE_ROW2_Y + 28, 4);
    lastWindText = w;
  }

  String nl = nextSunLabel();
  String nt = nextSunTimeText();
  if (nl != lastNextSunLabel || nt != lastNextSunTime) {
    tft.fillRect(18, PAGE_ROW3_Y + 24, 88, 30, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(nl, 18, PAGE_ROW3_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    if (useUsRegionFormat()) {
      int splitAt = nt.lastIndexOf(' ');
      String sunMain = splitAt > 0 ? nt.substring(0, splitAt) : nt;
      String sunSuffix = splitAt > 0 ? nt.substring(splitAt + 1) : "";
      tft.drawString(sunMain, 18, PAGE_ROW3_Y + 26, 4);
      if (sunSuffix.length() > 0) {
        int suffixX = 18 + tft.textWidth(sunMain, 4) + 3;
        tft.drawString(sunSuffix, suffixX, PAGE_ROW3_Y + 31, 2);
      }
    } else {
      tft.drawString(nt, 18, PAGE_ROW3_Y + 26, 4);
    }
    lastNextSunLabel = nl;
    lastNextSunTime = nt;
  }

  String k = kpText();
  if (k != lastKpText || dataDirty) {
    tft.fillRect(134, PAGE_ROW3_Y + 30, 88, 30, COL_PANEL);
    tft.setTextColor(COL_DIM, COL_PANEL);
    tft.drawString(TXT(L_WEATHER_KP), 134, PAGE_ROW3_Y + 8, 2);
    tft.setTextColor(COL_TEXT, COL_PANEL);
    tft.drawString(k, 134, PAGE_ROW3_Y + 28, 4);
    lastKpText = k;
  }
}

void drawMqttPageFull() {
  tft.fillScreen(COL_BG);
  drawTopBar(TXT(L_TAB_MQTT));
  drawNavBar();

  drawCard(8, PAGE_ROW1_Y, 224, 220, true);

  pageDirty = false;
  lastDrawnPage = PAGE_MQTT;
  lastMqttStatusText = "";
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) lastMqttLine[i] = "";
}

void updateMqttDynamic() {
  int y0 = PAGE_ROW1_Y + 12;
  int displayCount = min(mqttValueCount, 8);
  for (int i = 0; i < displayCount; i++) {
    String label = mqttLabels[i];
    String value = mqttDisplayValue(i);
    String combined = label + "|" + value + "|" + mqttUnits[i] + "|" + String(mqttValueEnabled[i] ? 1 : 0);
    if (combined != lastMqttLine[i]) {
      int y = y0 + i * 27;
      tft.fillRect(18, y, 204, 24, COL_PANEL);
      tft.setTextColor(COL_DIM, COL_PANEL);
      tft.drawString(label, 18, y, 1);
      tft.setTextColor(mqttValueEnabled[i] ? COL_TEXT : COL_DIM, COL_PANEL);
      tft.drawRightString(value, 222, y + 8, 2);
      lastMqttLine[i] = combined;
    }
  }
}

void drawStatusPageFull() {
  tft.fillScreen(COL_BG);
  drawNavBar();

  pageDirty = false;
  lastDrawnPage = PAGE_INFO;

  lastWifiText = "";
  lastSignalText = "";
  lastIpText = "";
  lastUptimeText = "";
  lastMqttStatusText = "";
  lastNetworkToggleText = "";
}

void drawSetupRow(int y, const String& label, const String& value, uint16_t valueColor = COL_TEXT) {
  tft.fillRect(12, y, 216, 18, COL_BG);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(label, 12, y, 2);
  tft.setTextColor(valueColor, COL_BG);
  tft.drawRightString(value, 228, y, 2);
}

struct StatusPageRow {
  const char* label;
  String value;
  uint16_t color;
};

const int STATUS_ROWS_START_Y = 6;
const int STATUS_ROW_HEIGHT = 24;

void updateStatusDynamic() {
  String wifiText = wifiStatusText();
  if (WiFi.status() == WL_CONNECTED) wifiText += " (" + WiFi.SSID() + ")";
  String signal = signalText();
  String ip = ipText();
  String uptime = uptimeText();
  String sync = lastSyncValueText();
  String mqttText = mqttStateText();
  String firmwareText = String(INFOTERM_APP_NAME) + " " + String(INFOTERM_APP_VERSION);
  String brokerText = mqttHost + ":" + String(mqttPort);
  String vpnNameText = vpnName.length() ? vpnName : String("-");
  String vpnTunnelText = vpnConnected ? vpnLocalAddress : String("-");
  StatusPageRow rows[] = {
    {TXT(L_STATUS_VERSION), firmwareText, COL_ACCENT},
    {TXT(L_STATUS_WIFI), wifiText, statusColor()},
    {TXT(L_STATUS_SIGNAL), signal, COL_TEXT},
    {TXT(L_STATUS_IP), ip, COL_TEXT},
    {TXT(L_STATUS_BROKER), brokerText, COL_TEXT},
    {TXT(L_TAB_MQTT), mqttText, mqttClient.connected() ? COL_GREEN : COL_TEXT},
    {TXT(L_STATUS_UPTIME), uptime, COL_TEXT},
    {TXT(L_STATUS_SYNC), sync, COL_TEXT},
    {"VPN", vpnStatusText(), vpnConnected ? COL_GREEN : COL_DIM},
    {"Name", vpnNameText, vpnConnected ? COL_TEXT : COL_DIM},
    {TXT(L_WEB_TUNNEL_IP), vpnTunnelText, vpnConnected ? COL_TEXT : COL_DIM},
  };
  const int rowCount = sizeof(rows) / sizeof(rows[0]);

  // Redraw ONLY the rows whose value/colour actually changed - never a
  // full-area fillRect (fixed 1.0.1). The old code wiped the whole status
  // area and repainted all nine rows whenever a single combined string
  // changed, and that string included the WiFi signal in dBm, which
  // fluctuates constantly - so the Info page rebuilt top-to-bottom every
  // couple of seconds (the reported flicker; this is the Info page, not the
  // widget pages the earlier fixes touched). drawSetupRow() already clears
  // its own row. drawStatusPageFull() empties lastIpText to force a one-time
  // full repaint of every row after a page switch.
  static String statusRowCache[11];  // must be >= rowCount
  bool fullRedraw = (lastIpText.length() == 0);
  lastIpText = "x";

  int y = STATUS_ROWS_START_Y;
  for (int i = 0; i < rowCount && i < 11; i++) {
    String key = rows[i].value + "|" + String(rows[i].color);
    if (fullRedraw || key != statusRowCache[i]) {
      statusRowCache[i] = key;
      drawSetupRow(y, rows[i].label, rows[i].value, rows[i].color);
    }
    y += STATUS_ROW_HEIGHT;
  }
}

// SoftAP onboarding portal screen (1.0.6): shown full-screen while the setup
// access point is active so a headless user knows what to connect to.
void drawSetupPortalPage() {
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(TXT(L_PORTAL_TITLE), 12, 20, 4);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(TXT(L_PORTAL_DISP_HINT), 12, 62, 2);
  int y = 112;
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("WLAN:", 12, y, 2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(portalApSsid, 92, y, 2);
  y += 30;
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(String(TXT(L_WEB_PASSWORD)) + ":", 12, y, 2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(INFOTERM_AP_PASSWORD, 92, y, 2);
  y += 30;
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("URL:", 12, y, 2);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString("http://192.168.4.1", 92, y, 2);
}

void drawCurrentPageFull() {
  if (portalActive) {
    drawSetupPortalPage();
    // Must mark the page as drawn: the loop() redraw trigger is
    // "pageDirty || lastDrawnPage != currentPage", and only the regular page
    // draw functions update lastDrawnPage. Without this the portal page was
    // redrawn (fillScreen) every loop iteration - visible as heavy flicker
    // (1.0.7 fix).
    lastDrawnPage = currentPage;
    return;
  }
  switch (currentPage) {
    case PAGE_HOME: drawHomePageFull(); break;
    case PAGE_TAB1: drawCustomTabPageFull(0); break;
    case PAGE_TAB2: drawCustomTabPageFull(1); break;
    case PAGE_TAB3: drawCustomTabPageFull(2); break;
    case PAGE_INFO: drawStatusPageFull(); break;
  }

}

void updateCurrentPageDynamic() {
  if (portalActive) return;

  switch (currentPage) {
    case PAGE_HOME: updateHomeDynamic(); break;
    case PAGE_TAB1: updateCustomTabDynamic(0); break;
    case PAGE_TAB2: updateCustomTabDynamic(1); break;
    case PAGE_TAB3: updateCustomTabDynamic(2); break;
    case PAGE_INFO: updateStatusDynamic(); break;
  }
}

// =========================================================
// NAVIGATION
// =========================================================
void handleNavTouch(int x, int y) {
  if (y < SCREEN_H - NAV_H) return;

  Page pages[5];
  String labels[5];
  int count = visibleNavPages(pages, labels, 5);
  if (count <= 0) return;

  int btnW = SCREEN_W / count;
  int idx = x / btnW;
  if (idx < 0 || idx >= count) return;

  Page newPage = pages[idx];
  if (newPage != currentPage) {
    currentPage = newPage;
    pageDirty = true;
  }
}

// WebGUI handlers and HTTP setup moved to the WebGUI module.
#include "webgui/WebGuiRuntime.inc"

// =========================================================
// SETUP / LOOP
// =========================================================
#include "wifi/WifiRuntime.inc"

// Application entry points are kept in a separate module.
#include "Application.inc"
