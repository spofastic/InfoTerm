#include "wifi/WifiModule.h"
#include "Version.h"

/******************************************************************************
 * InfoTerm
 * Module  : WiFi module scaffold
 * Version : 0.6.0
 *
 * WiFi and network connectivity scaffold. Existing WiFi connection lifecycle remains unchanged.
 ******************************************************************************/

namespace InfoTermWiFi {

void begin() {
  // Reserved for future module initialization.
}

void update() {
  // Reserved for future periodic module work.
}

const char* moduleName() {
  return "WiFi";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermWiFi
