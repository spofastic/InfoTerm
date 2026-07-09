#include "weather/WeatherModule.h"
#include "Version.h"

/******************************************************************************
 * InfoTerm
 * Module  : Weather module scaffold
 * Version : 0.6.0
 *
 * Weather provider scaffold. Existing weather fetching, parsing and DataPoint mirroring remains unchanged.
 ******************************************************************************/

namespace InfoTermWeather {

void begin() {
  // Reserved for future module initialization.
}

void update() {
  // Reserved for future periodic module work.
}

const char* moduleName() {
  return "Weather";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermWeather
