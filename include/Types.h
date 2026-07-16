#pragma once

/******************************************************************************
 * InfoTerm
 * Version : 0.0.7.5.1
 * Module  : Shared type definitions
 *
 * Types declared here must be available before the Arduino preprocessor
 * generates function prototypes. Keeping them in a header prevents compile
 * errors when functions use custom enum types in their signatures.
 ******************************************************************************/

enum HomeWidgetType {
  HOME_WIDGET_WEEK = 0,
  HOME_WIDGET_RAIN,
  HOME_WIDGET_OUTDOOR,
  HOME_WIDGET_KP,
  HOME_WIDGET_UV,
  HOME_WIDGET_WIND,
  HOME_WIDGET_SUN,
  HOME_WIDGET_MQTT_1,
  HOME_WIDGET_MQTT_2,
  HOME_WIDGET_MQTT_3,
  HOME_WIDGET_MQTT_4,
  HOME_WIDGET_MQTT_5,
  HOME_WIDGET_MQTT_6,
  HOME_WIDGET_MQTT_7,
  HOME_WIDGET_MQTT_8,
  HOME_WIDGET_MQTT_9,
  HOME_WIDGET_MQTT_10,
  HOME_WIDGET_MQTT_11,
  HOME_WIDGET_MQTT_12,
  HOME_WIDGET_DATAPOINT
};

enum Page {
  PAGE_HOME = 0,
  PAGE_TAB1 = 1,
  PAGE_TAB2 = 2,
  PAGE_TAB3 = 3,
  PAGE_TAB4 = 4,  // 1.0.12, issue #9 (numeric values are never persisted)
  PAGE_INFO = 5,
  // Compatibility aliases for existing internal helper functions.
  PAGE_WEATHER = PAGE_TAB1,
  PAGE_MQTT = PAGE_TAB2,
  PAGE_STATUS = PAGE_INFO
};
