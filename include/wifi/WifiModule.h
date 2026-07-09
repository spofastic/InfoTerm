#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : WiFi module scaffold
 * Version : 0.5.0
 *
 * WiFi and network connectivity scaffold. Existing WiFi connection lifecycle remains unchanged.
 ******************************************************************************/

namespace InfoTermWiFi {

void begin();
void update();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermWiFi
