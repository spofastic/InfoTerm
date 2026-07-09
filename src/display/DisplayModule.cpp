#include "display/DisplayModule.h"
#include "Version.h"

/*****************************************************************************/
/* InfoTerm                                                                  */
/* Version : 0.6.0                                                           */
/* Module  : Display module implementation                                   */
/*                                                                           */
/* This file currently contains only the safe display module scaffold.        */
/* Actual TFT rendering functions will be moved here gradually in future      */
/* releases after each step has been verified to build and run correctly.     */
/*****************************************************************************/

namespace InfoTermDisplay {

void initializeModule() {
  // Reserved for future display subsystem initialization.
  // Keeping this no-op avoids runtime changes in 0.4.1.
}

void begin() {
  initializeModule();
}

void update() {
  // Reserved for future periodic display subsystem work.
}

const char* moduleName() {
  return "Display";
}

const char* moduleVersion() {
  return INFOTERM_APP_VERSION;
}

}
