#pragma once
#include <Arduino.h>

/******************************************************************************
 * InfoTerm
 * Version : 0.0.6.4
 * Modul   : Language
 *
 * Zentrale Sprachdatei für alle sichtbaren Texte der Weboberfläche und Anzeige.
 * Deutsch ist die Standardsprache, Englisch ist vorbereitet.
 ******************************************************************************/

enum UiLanguage {
  LANG_DE = 0,
  LANG_EN = 1
};

extern UiLanguage uiLanguage;

enum LanguageKey {
  L_TAB_START,
  L_TAB_WEATHER,
  L_TAB_MQTT,
  L_TAB_STATUS,
  L_WIDGET_WEEK,
  L_WIDGET_RAIN,
  L_WIDGET_OUTDOOR,
  L_WIDGET_KP,
  L_WIDGET_UV,
  L_WIDGET_WIND,
  L_WIDGET_SUN,
  L_SUN,
  L_SUNRISE,
  L_SUNSET,
  L_WEATHER_OUTDOOR,
  L_WEATHER_RAIN,
  L_WEATHER_UV,
  L_WEATHER_WIND,
  L_WEATHER_KP,
  L_STATUS_WIFI,
  L_STATUS_SIGNAL,
  L_STATUS_IP,
  L_STATUS_UPTIME,
  L_STATUS_NETWORK,
  L_STATUS_FIRMWARE,
  L_STATUS_VERSION,
  L_STATUS_BROKER,
  L_STATUS_SYNC,
  L_STATUS_ENABLED,
  L_STATUS_DISABLED,
  L_STATUS_ONLINE,
  L_STATUS_OFFLINE,
  L_STATUS_MQTT_OFF,
  L_BOOT_SYNCING_TIME,
  L_WEB_INTRO,
  L_WEB_LANGUAGE,
  L_WEB_LANGUAGE_DE,
  L_WEB_LANGUAGE_EN,
  L_WEB_MQTT_WEATHER_TITLE,
  L_WEB_MQTT_WEATHER_DESC,
  L_WEB_USE_MQTT_WEATHER,
  L_WEB_USE_OPENMETEO_FALLBACK,
  L_WEB_BROKER,
  L_WEB_PORT,
  L_WEB_USER,
  L_WEB_PASSWORD,
  L_WEB_OBSERVATION_TOPIC,
  L_WEB_THEME_TITLE,
  L_WEB_THEME_DESC,
  L_WEB_ACCENT,
  L_WEB_BORDER_COLOR,
  L_WEB_TEXT_COLOR,
  L_WEB_THEME,
  L_WEB_SETTINGS,
  L_WEB_SETTINGS_DESC,
  L_WEB_GENERAL,
  L_WEB_DEVICE_NAME,
  L_WEB_UNITS,
  L_WEB_DATE_FORMAT,
  L_WEB_TIMEZONE,
  L_WEB_FIELD,
  L_WEB_LOCATION,
  L_WEB_LOCATION_DESC,
  L_WEB_LOCATION_NAME,
  L_WEB_LATITUDE,
  L_WEB_LONGITUDE,
  L_WEB_MQTT_VALUES_TITLE,
  L_WEB_DISPLAY_VALUES,
  L_WEB_DISPLAY_VALUES_DESC,
  L_WEB_LABEL,
  L_WEB_UNIT,
  L_WEB_TOPIC,
  L_WEB_SHOW,
  L_WEB_WIDGETS_TITLE,
  L_WEB_WIDGETS_DESC,
  L_WIDGET_EMPTY,
  L_LEVEL_LOW,
  L_LEVEL_MEDIUM,
  L_LEVEL_MODERATE,
  L_LEVEL_HIGH,
  L_LEVEL_VERY_HIGH,
  L_LEVEL_EXTREME,
  L_WEB_SAVE,
  L_WEB_SAVED_TITLE,
  L_WEB_SAVED_DESC,
  L_WEB_BACK,
  L_WEB_PASSWORD_KEEP_HINT,
  L_WEB_PASSWORD_CLEAR,
  L_COUNT
};

struct LanguageEntry {
  const char* de;
  const char* en;
};

static const LanguageEntry LANGUAGE_TABLE[L_COUNT] = {
  {"Home", "Home"},
  {"Wetter", "Weather"},
  {"MQTT", "MQTT"},
  {"Info", "Info"},
  {"Woche", "Week"},
  {"Regen", "Rain"},
  {"Temperatur", "Temperature"},
  {"KP-Index", "KP index"},
  {"UV-Index", "UV index"},
  {"Wind", "Wind"},
  {"Sonne", "Sun"},
  {"Sonne", "Sun"},
  {"Sonnenaufgang", "Sunrise"},
  {"Sonnenuntergang", "Sunset"},
  {"Temperatur", "Temperature"},
  {"Regen", "Rain"},
  {"UV-Index", "UV index"},
  {"Wind", "Wind"},
  {"KP-Index", "KP index"},
  {"WLAN", "WiFi"},
  {"Signal", "Signal"},
  {"IP-Adresse", "IP address"},
  {"Laufzeit", "Uptime"},
  {"Netzwerk", "Network"},
  {"Firmware", "Firmware"},
  {"Version", "Version"},
  {"Broker", "Broker"},
  {"Synchronisierung", "Sync"},
  {"Aktiviert", "Enabled"},
  {"Deaktiviert", "Disabled"},
  {"Online", "Online"},
  {"Offline", "Offline"},
  {"MQTT aus", "MQTT off"},
  {"Synchronisiere Zeit...", "Syncing time..."},
  {"Passe InfoTerm mit Anzeigen, Design und praktischen Informationsfunktionen an.", "Customize InfoTerm with widgets, design and practical display features."},
  {"Sprache", "Language"},
  {"Deutsch", "German"},
  {"Englisch", "English"},
  {"MQTT Broker / Wetter", "MQTT Broker / Weather"},
  {"MQTT versorgt den Wetter-Tab bevorzugt. Wenn keine frischen MQTT-Wetterdaten eintreffen, wird bei aktivierter Ersatzquelle die Standard-Wetterquelle verwendet.", "MQTT is preferred for the Weather tab. If no fresh MQTT weather data arrives and fallback is enabled, the default weather source is used."},
  {"MQTT-Wetterstationswerte verwenden", "Use MQTT weather station values"},
  {"Open-Meteo als Ersatz verwenden, wenn MQTT nicht verfügbar ist", "Use Open-Meteo as fallback when MQTT is unavailable"},
  {"MQTT-Broker IP / Host", "MQTT broker IP / host"},
  {"Port", "Port"},
  {"Benutzer", "User"},
  {"Passwort", "Password"},
  {"WeatherFlow Beobachtungs-JSON-Topic", "WeatherFlow observation JSON topic"},
  {"Design", "Design"},
  {"Darstellung, Farben und Widget-Rahmen.", "Appearance, colors and widget borders."},
  {"Akzentfarbe", "Accent color"},
  {"Rahmenfarbe", "Border color"},
  {"Textfarbe", "Text color"},
  {"Design", "Design"},
  {"Einstellungen", "Settings"},
  {"Grundverhalten und Anzeigeeinstellungen.", "Basic behavior and display settings."},
  {"Allgemein", "General"},
  {"Gerätename", "Device name"},
  {"Einheitensystem", "Unit system"},
  {"Datumsformat", "Date format"},
  {"Zeitzone", "Time zone"},
  {"Feld", "Slot"},
  {"Standort", "Location"},
  {"Wird für Wetterdaten und Sonnenzeiten verwendet.", "Used for weather data and sunrise/sunset times."},
  {"Standortname", "Location name"},
  {"Breitengrad", "Latitude"},
  {"Längengrad", "Longitude"},
  {"MQTT Datenpunkte", "MQTT DataPoints"},
  {"Anzeigewerte", "Display values"},
  {"Bezeichnung, Topic und Sichtbarkeit je Wert.", "Label, topic and visibility per value."},
  {"Bezeichnung", "Label"},
  {"Einheit", "Unit"},
  {"Topic", "Topic"},
  {"Anzeigen", "Show"},
  {"Widget-Auswahl", "Widget selection"},
  {"Wähle aus, welche Widgets in den sechs Home-Feldern angezeigt werden.", "Choose which widgets are shown in the six home slots."},
  {"Leer", "Empty"},
  {"Niedrig", "Low"},
  {"Mittel", "Medium"},
  {"Moderat", "Moderate"},
  {"Hoch", "High"},
  {"Sehr hoch", "Very High"},
  {"Extrem", "Extreme"},
  {"Speichern", "Save"},
  {"Gespeichert", "Saved"},
  {"Die Einstellungen wurden übernommen. Der InfoTerm aktualisiert sich jetzt.", "The settings have been applied. InfoTerm is updating now."},
  {"Zurück", "Back"},
  {"Gespeichert – leer lassen, um es zu behalten", "Stored – leave empty to keep it"},
  {"Gespeichertes Passwort entfernen", "Remove stored password"}
};

inline const char* TXT(LanguageKey key) {
  if (key < 0 || key >= L_COUNT) return "";
  return (uiLanguage == LANG_EN) ? LANGUAGE_TABLE[key].en : LANGUAGE_TABLE[key].de;
}

inline const char* languageCode() {
  return (uiLanguage == LANG_EN) ? "en" : "de";
}

inline const char* localizedWeekdayShort(int tmWday) {
  static const char* de[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
  static const char* en[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (tmWday < 0 || tmWday > 6) return "";
  return (uiLanguage == LANG_EN) ? en[tmWday] : de[tmWday];
}

inline const char* localizedWeekdayLong(int tmWday) {
  static const char* de[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
  static const char* en[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  if (tmWday < 0 || tmWday > 6) return "";
  return (uiLanguage == LANG_EN) ? en[tmWday] : de[tmWday];
}

inline const char* localizedMonthShort(int tmMon) {
  static const char* de[] = {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};
  static const char* en[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (tmMon < 0 || tmMon > 11) return "";
  return (uiLanguage == LANG_EN) ? en[tmMon] : de[tmMon];
}

inline UiLanguage languageFromCode(const String& value) {
  return value == "en" ? LANG_EN : LANG_DE;
}
