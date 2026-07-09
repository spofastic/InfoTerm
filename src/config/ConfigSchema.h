#pragma once

#include <Arduino.h>

// InfoTerm configuration schema version.
// This is intentionally independent from the firmware version.
// Increase only when the persisted configuration format changes.
#ifndef INFOTERM_CONFIG_SCHEMA_VERSION
#define INFOTERM_CONFIG_SCHEMA_VERSION 1
#endif

#define INFOTERM_CONFIG_VERSION_KEY "configVersion"

// Deliberately NO config CRC (decided 0.9.25): an earlier configCrc key and
// crc32 plumbing existed here but were never written or checked. Integrity
// of the stored bytes is already NVS's job (per-entry CRCs in ESP-IDF), and
// value-level plausibility runs through sanitizeGeneralConfig() on both the
// boot-load and backup-import paths since 0.9.14 - a second checksum on top
// would add coupling without a failure mode it could catch.

namespace InfoTermConfigSchema {
  constexpr uint16_t CURRENT = INFOTERM_CONFIG_SCHEMA_VERSION;
  constexpr const char* VERSION_KEY = INFOTERM_CONFIG_VERSION_KEY;

  inline bool needsMigration(uint16_t storedVersion) {
    return storedVersion > 0 && storedVersion < CURRENT;
  }

  inline bool isUnknownOrLegacy(uint16_t storedVersion) {
    return storedVersion == 0;
  }
}
