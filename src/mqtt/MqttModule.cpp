/******************************************************************************
 * InfoTerm
 * Module  : MQTT module scaffold
 * Version : 0.6.1
 *
 * The MQTT implementation still lives in the legacy firmware file. This scaffold
 * provides a stable module boundary for the next extraction steps without
 * changing runtime behavior.
 ******************************************************************************/

#include "mqtt/MqttModule.h"
#include "Version.h"

namespace InfoTermMqtt {

void begin() {
  // Reserved for the future MQTT initialization handover.
}

void loop() {
  // Reserved for the future MQTT runtime handover.
}

const char* moduleName() {
  return "MQTT";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermMqtt
