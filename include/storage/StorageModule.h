#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : Storage module scaffold
 * Version : 0.5.1
 *
 * This header introduces the namespace that will later own persistent settings,
 * Preferences/NVS access and LittleFS helper logic. In this release it is
 * intentionally non-invasive and does not replace the existing stable legacy
 * persistence code yet.
 ******************************************************************************/

namespace InfoTermStorage {

void begin();
void load();
void save();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermStorage
