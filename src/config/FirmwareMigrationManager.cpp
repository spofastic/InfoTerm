#include "FirmwareMigrationManager.h"
#include "../system/Logger.h"

namespace InfoTermFirmwareMigrationManager {

bool begin() {
  Logger::info("[Migration] FirmwareMigrationManager initialized");
  return true;
}

bool isSchemaCompatible(uint16_t storedSchema) {
  return storedSchema <= InfoTermConfigSchema::CURRENT;
}

MigrationStatus checkAndMigrate(uint16_t storedSchema) {
  MigrationStatus status;
  status.ok = true;
  status.migrated = false;
  status.factoryResetRequired = false;
  status.fromSchema = storedSchema;
  status.toSchema = InfoTermConfigSchema::CURRENT;

  if (storedSchema == 0) {
    status.migrated = true;
    status.message = "Legacy configuration detected; preserve existing values and mark current schema.";
    Logger::info("[Migration] Legacy configuration detected");
    return status;
  }

  if (storedSchema > InfoTermConfigSchema::CURRENT) {
    status.ok = false;
    status.factoryResetRequired = true;
    status.message = "Configuration schema is newer than firmware; manual intervention required.";
    Logger::error("[Migration] config schema newer than firmware");
    return status;
  }

  if (storedSchema < InfoTermConfigSchema::CURRENT) {
    status.migrated = true;
    status.message = "Configuration migration required and accepted.";
    Logger::warning("[Migration] Config migration required");
    return status;
  }

  status.message = "Configuration schema is current.";
  Logger::debug("[Migration] Config schema current");
  return status;
}

const char* statusMessage(const MigrationStatus& status) {
  if (!status.ok) return "migration-error";
  if (status.factoryResetRequired) return "factory-reset-required";
  if (status.migrated) return "migrated";
  return "current";
}

}
