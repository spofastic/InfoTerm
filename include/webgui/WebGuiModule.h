#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : WebGUI module scaffold
 * Version : 0.6.5
 *
 * WebGUI and HTTP page routing module. Runtime handlers and LittleFS helpers were migrated from InfoTerm.ino in version 0.6.5.
 ******************************************************************************/

namespace InfoTermWebGui {

void begin();
void update();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermWebGui
