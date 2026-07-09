/******************************************************************************
 * InfoTerm
 * Module  : Storage module scaffold
 * Version : 0.5.1
 *
 * The storage and configuration implementation still lives in the legacy
 * firmware file. This scaffold provides a stable module boundary for the next
 * extraction steps without changing runtime behavior.
 ******************************************************************************/

#include "storage/StorageModule.h"
#include "Version.h"

namespace InfoTermStorage {

void begin() {
  // Reserved for the future storage initialization handover.
}

void load() {
  // Reserved for the future configuration loading handover.
}

void save() {
  // Reserved for the future configuration saving handover.
}

const char* moduleName() {
  return "Storage";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermStorage
