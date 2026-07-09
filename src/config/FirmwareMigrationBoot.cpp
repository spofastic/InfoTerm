#include "FirmwareMigrationBoot.h"
#include "../system/Logger.h"

namespace InfoTermFirmwareMigrationBoot {

static BootMigrationState bootState = {false, true, false, false, 0, INFOTERM_CONFIG_SCHEMA_VERSION, "not-started"};

bool begin(uint16_t storedSchema) {
  InfoTermFirmwareMigrationManager::begin();
  auto result = InfoTermFirmwareMigrationManager::checkAndMigrate(storedSchema);

  bootState.initialized = true;
  bootState.ok = result.ok;
  bootState.migrated = result.migrated;
  bootState.factoryResetRequired = result.factoryResetRequired;
  bootState.storedSchema = result.fromSchema;
  bootState.currentSchema = result.toSchema;
  bootState.message = result.message;

  Logger::info(String("[MigrationBoot] schema ") + bootState.storedSchema +
               " -> " + bootState.currentSchema + " status=" + bootState.message);

  return bootState.ok && !bootState.factoryResetRequired;
}

const BootMigrationState& state() {
  return bootState;
}

bool isOk() {
  return bootState.initialized && bootState.ok;
}

bool wasMigrated() {
  return bootState.initialized && bootState.migrated;
}

bool needsFactoryReset() {
  return bootState.initialized && bootState.factoryResetRequired;
}

}
