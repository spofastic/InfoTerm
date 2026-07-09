#pragma once

/******************************************************************************
 * InfoTerm
 * Module  : Widget module scaffold
 * Version : 0.6.4
 *
 * This header owns the widget module namespace and marks the 0.6.4 migration of
 * the legacy sprite based widget rendering helpers into the widget module area.
 * Widget assignments and WebGUI configuration remain compatible.
 ******************************************************************************/

namespace InfoTermWidgets {

void begin();
void update();
void render();
const char* moduleName();
const char* moduleVersion();

}  // namespace InfoTermWidgets
