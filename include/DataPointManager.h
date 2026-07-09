#pragma once

/******************************************************************************
 * InfoTerm
 * Version : 0.0.6.4
 * Module  : DataPointManager
 *
 * Central registry for runtime values used by weather, MQTT, system status and
 * future data providers. Version 0.0.6.4 keeps the manager compatible with the existing
 * widget system and exposes it as a read-only WebGUI diagnostic view.
 ******************************************************************************/

#include <Arduino.h>

#ifndef INFOTERM_MAX_DATAPOINTS
#define INFOTERM_MAX_DATAPOINTS 96
#endif

enum DataPointSource {
  DATA_SOURCE_UNKNOWN = 0,
  DATA_SOURCE_WEATHER,
  DATA_SOURCE_MQTT,
  DATA_SOURCE_SYSTEM,
  DATA_SOURCE_INTERNAL
};

struct DataPoint {
  String id;
  String label;
  String value;
  String unit;
  DataPointSource source;
  bool valid;
  unsigned long lastUpdateMs;
};

class DataPointManager {
 public:
  DataPointManager() : count_(0) {}

  void clear() {
    count_ = 0;
  }

  bool registerPoint(const String& id, const String& label, const String& unit, DataPointSource source) {
    int idx = indexOf(id);
    if (idx < 0) {
      if (count_ >= INFOTERM_MAX_DATAPOINTS) return false;
      idx = count_++;
      points_[idx].id = id;
      points_[idx].value = "--";
      points_[idx].valid = false;
      points_[idx].lastUpdateMs = 0;
    }
    points_[idx].label = label;
    points_[idx].unit = unit;
    points_[idx].source = source;
    return true;
  }

  bool setValue(const String& id, const String& value, const String& unit = "", bool valid = true) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    points_[idx].value = value;
    if (unit.length() > 0 || points_[idx].unit.length() == 0) points_[idx].unit = unit;
    points_[idx].valid = valid;
    points_[idx].lastUpdateMs = millis();
    return true;
  }

  bool setLabel(const String& id, const String& label) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    points_[idx].label = label;
    return true;
  }

  bool setUnit(const String& id, const String& unit) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    points_[idx].unit = unit;
    return true;
  }

  DataPoint* get(const String& id) {
    int idx = indexOf(id);
    return idx >= 0 ? &points_[idx] : nullptr;
  }

  const DataPoint* get(const String& id) const {
    int idx = indexOf(id);
    return idx >= 0 ? &points_[idx] : nullptr;
  }

  DataPoint* at(int index) {
    if (index < 0 || index >= count_) return nullptr;
    return &points_[index];
  }

  const DataPoint* at(int index) const {
    if (index < 0 || index >= count_) return nullptr;
    return &points_[index];
  }

  int count() const {
    return count_;
  }

 private:
  int indexOf(const String& id) const {
    for (int i = 0; i < count_; i++) {
      if (points_[i].id == id) return i;
    }
    return -1;
  }

  DataPoint points_[INFOTERM_MAX_DATAPOINTS];
  int count_;
};
