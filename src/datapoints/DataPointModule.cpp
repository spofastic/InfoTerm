#include "datapoints/DataPointModule.h"
#include "Version.h"

/******************************************************************************
 * InfoTerm
 * Module  : DataPoint module scaffold
 * Version : 0.6.3
 *
 * DataPoint module scaffold with runtime helpers migrated in version 0.6.3.
 * Existing behavior remains unchanged; the global DataPointManager is still
 * used as the authoritative runtime store.
 ******************************************************************************/

namespace InfoTermDataPoints {

void begin() {
  // Reserved for future DataPoint subsystem initialization.
}

void registerDefaults() {
  // Reserved for future weather, MQTT and system DataPoint registration.
}

void updateRuntimeValues() {
  // Reserved for future DataPoint update orchestration.
}

const char* moduleName() {
  return "DataPoints";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermDataPoints
