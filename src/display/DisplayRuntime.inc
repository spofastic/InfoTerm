/******************************************************************************
 * InfoTerm
 * Version : 0.6.0
 * Module  : Display runtime migration
 *
 * This file contains legacy runtime functions moved out of InfoTerm.ino as the
 * first productive migration step. It is included into the legacy compilation
 * unit to keep access to the existing stable global runtime state while reducing
 * the size and complexity of the main firmware file.
 ******************************************************************************/

void drawCard(int x, int y, int w, int h, bool accent = false) {
  tft.fillRoundRect(x, y, w, h, 10, COL_PANEL);
  if (widgetBordersEnabled) {
    tft.drawRoundRect(x, y, w, h, 10, COL_STROKE);
  }
}

void drawTopBar(const String& title) {
  // Header intentionally disabled for the compact InfoTerm display layout.
  (void)title;
}
