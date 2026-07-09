#pragma once

#include <Arduino.h>
#include "FirmwareMigrationManager.h"

// Boot-time integration adapter for the firmware migration path.
// This keeps the integration compile-safe while the existing configuration
// loading code is migrated section by section.
namespace InfoTermFirmwareMigrationBoot {

  struct BootMigrationState {
    bool initialized;
    bool ok;
    bool migrated;
    bool factoryResetRequired;
    uint16_t storedSchema;
    uint16_t currentSchema;
    String message;
  };

  bool begin(uint16_t storedSchema = 0);
  const BootMigrationState& state();
  bool isOk();
  bool wasMigrated();
  bool needsFactoryReset();

}
