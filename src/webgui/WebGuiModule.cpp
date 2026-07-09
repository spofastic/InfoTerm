#include "webgui/WebGuiModule.h"
#include "Version.h"

/******************************************************************************
 * InfoTerm
 * Module  : WebGUI module scaffold
 * Version : 0.6.5
 *
 * WebGUI and HTTP page routing module. Runtime handlers and LittleFS helpers were migrated from InfoTerm.ino in version 0.6.5.
 ******************************************************************************/

namespace InfoTermWebGui {

void begin() {
  // Runtime handlers are included from WebGuiRuntime.inc in InfoTerm.ino.
}

void update() {
  // Reserved for future asynchronous WebGUI work.
}

const char* moduleName() {
  return "WebGUI";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermWebGui
