#include "widgets/WidgetModule.h"
#include "Version.h"

/******************************************************************************
 * InfoTerm
 * Module  : Widget module scaffold
 * Version : 0.6.4
 *
 * Widget module with runtime helpers migrated in version 0.6.4.
 * Rendering remains functionally unchanged; the legacy sprite based widget
 * engine now lives in src/widgets/WidgetRuntime.inc.
 ******************************************************************************/

namespace InfoTermWidgets {

void begin() {
  // Reserved for future widget subsystem initialization.
}

void update() {
  // Reserved for future widget state updates and DataPoint binding refresh.
}

void render() {
  // Reserved for future renderer dispatch.
}

const char* moduleName() {
  return "Widgets";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}  // namespace InfoTermWidgets
