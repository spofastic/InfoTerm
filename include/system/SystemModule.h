#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : System module scaffold
 * Version : 0.5.0
 *
 * System services scaffold for time, NTP, display sleep, backlight and power-management responsibilities. Existing runtime logic remains unchanged.
 ******************************************************************************/

namespace InfoTermSystem {

void begin();
void update();

void tickTime();
void updatePowerState();
void updateBacklight();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermSystem
