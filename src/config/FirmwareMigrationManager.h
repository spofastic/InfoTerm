#pragma once

#include <Arduino.h>
#include "ConfigSchema.h"

namespace InfoTermFirmwareMigrationManager {

struct MigrationStatus {
  bool ok;
  bool migrated;
  bool factoryResetRequired;
  uint16_t fromSchema;
  uint16_t toSchema;
  String message;
};

bool begin();
MigrationStatus checkAndMigrate(uint16_t storedSchema);
bool isSchemaCompatible(uint16_t storedSchema);
const char* statusMessage(const MigrationStatus& status);

}
