#pragma once

/*****************************************************************************/
/* InfoTerm                                                                  */
/* Version : 0.5.0                                                           */
/* Module  : Display module interface                                        */
/*                                                                           */
/* First safe step towards a dedicated display subsystem.                     */
/*                                                                           */
/* This module is intentionally small in 0.4.1. It introduces a stable        */
/* namespace and interface without changing the existing rendering flow.      */
/* The legacy display code remains in InfoTerm.cpp and will be moved in       */
/* later releases one responsibility at a time.                               */
/*****************************************************************************/

namespace InfoTermDisplay {

void initializeModule();
void begin();
void update();
const char* moduleName();
const char* moduleVersion();

}
