#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : MQTT module scaffold
 * Version : 0.5.1
 *
 * This header introduces the namespace that will later own MQTT connection,
 * subscription and DataPoint update logic. In this release it is intentionally
 * non-invasive and does not replace the existing stable legacy MQTT code yet.
 ******************************************************************************/

namespace InfoTermMqtt {

void begin();
void loop();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermMqtt
