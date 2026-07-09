// InfoTerm Storage runtime migration
//
// This file is intentionally included by InfoTerm.cpp during the gradual
// PlatformIO modularization. It keeps the existing NVS/Preferences behavior
// unchanged while moving storage-specific helpers out of the main sketch.

// Persistent configuration keys.
//
// ESP32 Preferences/NVS keys are limited to 15 characters. Earlier cfg2_* keys
// were too long for several settings (for example mqttValueCount and
// tabEnabled0), so some writes silently failed and old configuration values were
// loaded again after flashing. The current storage layer maps logical setting
// names to short deterministic keys using a small FNV-1a hash. This keeps the
// public code readable while all physical NVS keys stay well below the limit.
static const char* CONFIG_SAVE_MARKER = "c3saved";
static const char* CONFIG_OLD_SAVE_MARKER = "cfg2Saved";
static const char* CONFIG_PREFIX = "c";
static const char* LEGACY_CONFIG_PREFIX = "cfg_";

// Explicit short keys for settings that must survive firmware updates reliably.
// They are read before hashed/legacy keys and remain below the ESP32 NVS limit.
static const char* CFG_LOCATION_NAME_KEY = "gLoc";
static const char* CFG_LATITUDE_KEY      = "gLat";
static const char* CFG_LONGITUDE_KEY     = "gLng";
static const char* CFG_SLEEP_MIN_KEY     = "gSlp";

static uint32_t cfgHash(const String& base) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < base.length(); i++) {
    hash ^= (uint8_t)base[i];
    hash *= 16777619UL;
  }
  return hash;
}

static String cfgKey(const String& base) {
  char buf[12];
  snprintf(buf, sizeof(buf), "c%08lx", (unsigned long)cfgHash(base));
  return String(buf);
}

static String oldCfg2Key(const String& base) {
  return String("cfg2_") + base;
}

static String legacyCfgKey(const String& base) {
  return String(LEGACY_CONFIG_PREFIX) + base;
}

static bool cfgHasSavedConfig() {
  return prefs.getBool(CONFIG_SAVE_MARKER, false);
}

static bool cfgHasOldSavedConfig() {
  return prefs.getBool(CONFIG_OLD_SAVE_MARKER, false);
}

// Configuration lookup order:
// 1. Current short hashed keys (cXXXXXXXX)
// 2. Previous cfg2_* keys, but only when they physically exist
// 3. Legacy keys only before a current short-key config has been saved
//
// This prevents stale legacy values from overriding the current config while
// still allowing one-time migration from older firmware builds.
static String cfgGetString(const String& base, const String& fallbackKey, const String& def) {
  String key = cfgKey(base);
  if (prefs.isKey(key.c_str())) return prefs.getString(key.c_str(), def);

  String oldCfg2 = oldCfg2Key(base);
  if (prefs.isKey(oldCfg2.c_str())) return prefs.getString(oldCfg2.c_str(), def);

  if (cfgHasSavedConfig()) return def;

  if (!cfgHasOldSavedConfig() && prefs.isKey(fallbackKey.c_str())) return prefs.getString(fallbackKey.c_str(), def);

  String oldKey = legacyCfgKey(base);
  if (!cfgHasOldSavedConfig() && prefs.isKey(oldKey.c_str())) return prefs.getString(oldKey.c_str(), def);

  return def;
}

static bool cfgGetBool(const String& base, const String& fallbackKey, bool def) {
  String key = cfgKey(base);
  if (prefs.isKey(key.c_str())) return prefs.getBool(key.c_str(), def);

  String oldCfg2 = oldCfg2Key(base);
  if (prefs.isKey(oldCfg2.c_str())) return prefs.getBool(oldCfg2.c_str(), def);

  if (cfgHasSavedConfig()) return def;

  if (!cfgHasOldSavedConfig() && prefs.isKey(fallbackKey.c_str())) return prefs.getBool(fallbackKey.c_str(), def);

  String oldKey = legacyCfgKey(base);
  if (!cfgHasOldSavedConfig() && prefs.isKey(oldKey.c_str())) return prefs.getBool(oldKey.c_str(), def);

  return def;
}

static int cfgGetInt(const String& base, const String& fallbackKey, int def) {
  String key = cfgKey(base);
  if (prefs.isKey(key.c_str())) return prefs.getInt(key.c_str(), def);

  String oldCfg2 = oldCfg2Key(base);
  if (prefs.isKey(oldCfg2.c_str())) return prefs.getInt(oldCfg2.c_str(), def);

  if (cfgHasSavedConfig()) return def;

  if (!cfgHasOldSavedConfig() && prefs.isKey(fallbackKey.c_str())) return prefs.getInt(fallbackKey.c_str(), def);

  String oldKey = legacyCfgKey(base);
  if (!cfgHasOldSavedConfig() && prefs.isKey(oldKey.c_str())) return prefs.getInt(oldKey.c_str(), def);

  return def;
}

static float cfgGetFloat(const String& base, const String& fallbackKey, float def) {
  String key = cfgKey(base);
  if (prefs.isKey(key.c_str())) return prefs.getFloat(key.c_str(), def);

  String oldCfg2 = oldCfg2Key(base);
  if (prefs.isKey(oldCfg2.c_str())) return prefs.getFloat(oldCfg2.c_str(), def);

  if (cfgHasSavedConfig()) return def;

  if (!cfgHasOldSavedConfig() && prefs.isKey(fallbackKey.c_str())) return prefs.getFloat(fallbackKey.c_str(), def);

  String oldKey = legacyCfgKey(base);
  if (!cfgHasOldSavedConfig() && prefs.isKey(oldKey.c_str())) return prefs.getFloat(oldKey.c_str(), def);

  return def;
}

static void cfgPutString(const String& base, const String& value) {
  prefs.putString(cfgKey(base).c_str(), value);
}

static void cfgPutBool(const String& base, bool value) {
  prefs.putBool(cfgKey(base).c_str(), value);
}

static void cfgPutInt(const String& base, int value) {
  prefs.putInt(cfgKey(base).c_str(), value);
}

static void cfgPutFloat(const String& base, float value) {
  prefs.putFloat(cfgKey(base).c_str(), value);
}

static void markConfigSaved() {
  prefs.putBool(CONFIG_SAVE_MARKER, true);
}
