#pragma once

#include <Arduino.h>
#include "../../include/Version.h"

namespace InfoTermVersionManager {
  inline const char* appName() { return INFOTERM_APP_NAME; }
  inline const char* appVersion() { return INFOTERM_APP_VERSION; }
  inline String fullVersion() { return String(INFOTERM_APP_NAME) + " " + String(INFOTERM_APP_VERSION); }
  inline String webVersionLabel() { return String("v") + String(INFOTERM_APP_VERSION); }
}
