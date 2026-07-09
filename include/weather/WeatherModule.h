#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : Weather module scaffold
 * Version : 0.5.0
 *
 * Weather provider scaffold. Existing weather fetching, parsing and DataPoint mirroring remains unchanged.
 ******************************************************************************/

namespace InfoTermWeather {

void begin();
void update();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermWeather
