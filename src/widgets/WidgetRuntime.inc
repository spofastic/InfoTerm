/******************************************************************************
 * InfoTerm
 * Module  : Widget runtime helpers
 * Version : 0.6.4
 *
 * Migrated from InfoTerm.ino in version 0.6.4. Contains the sprite based
 * widget rendering engine for home and custom tab widgets.
 ******************************************************************************/

void makeSpriteCard(TFT_eSprite& spr, int w, int h, bool accent = false) {
  spr.setColorDepth(16);
  spr.createSprite(w, h);
  spr.fillSprite(COL_BG);
  spr.fillRoundRect(0, 0, w, h, 10, COL_PANEL);
  if (widgetBordersEnabled) {
    spr.drawRoundRect(0, 0, w, h, 10, COL_STROKE);
  }
}

void pushSpriteAndDelete(TFT_eSprite& spr, int x, int y) {
  // COL_BG doubles as the transparency key so the rounded card corners
  // blend into the page background. If the card fill is the same color as
  // the page background (night mode: both black), that key would make the
  // entire card interior transparent - stale pixels (old clock digits)
  // would never be overwritten. Corners need no blending in that case, so
  // push opaque instead.
  if (COL_PANEL == COL_BG) {
    spr.pushSprite(x, y);
  } else {
    spr.pushSprite(x, y, COL_BG);
  }
  spr.deleteSprite();
}

void drawCleanSunIcon(TFT_eSprite& spr, int cx, int cy, uint16_t c) {
  spr.fillCircle(cx, cy, 4, c);
  spr.drawLine(cx, cy - 9, cx, cy - 7, c);
  spr.drawLine(cx, cy + 7, cx, cy + 9, c);
  spr.drawLine(cx - 9, cy, cx - 7, cy, c);
  spr.drawLine(cx + 7, cy, cx + 9, cy, c);
  spr.drawLine(cx - 6, cy - 6, cx - 5, cy - 5, c);
  spr.drawLine(cx + 5, cy - 5, cx + 6, cy - 6, c);
  spr.drawLine(cx - 6, cy + 6, cx - 5, cy + 5, c);
  spr.drawLine(cx + 5, cy + 5, cx + 6, cy + 6, c);
}

void drawMoonIcon(TFT_eSprite& spr, int cx, int cy, uint16_t c) {
  spr.fillCircle(cx, cy, 6, c);
  spr.fillCircle(cx + 4, cy - 2, 6, COL_PANEL);
}

int drawWrappedTextLimited(int x, int y, int maxW, const String& text, int font, uint16_t fg, uint16_t bg, int maxLines) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(fg, bg);

  const int lineH = tft.fontHeight(font) + 2;
  String line = "";
  String word = "";
  int linesDrawn = 0;

  auto flushLine = [&]() {
    if (linesDrawn >= maxLines) return;
    if (line.length() > 0) tft.drawString(line, x, y, font);
    y += lineH;
    line = "";
    linesDrawn++;
  };

  auto placeWordOnEmptyLine = [&]() {
    if (word.length() == 0 || linesDrawn >= maxLines) return;

    while (tft.textWidth(word, font) > maxW && word.length() > 1) {
      int cut = word.length();
      while (cut > 1 && tft.textWidth(word.substring(0, cut), font) > maxW) cut--;
      if (linesDrawn >= maxLines) return;
      tft.drawString(word.substring(0, cut), x, y, font);
      y += lineH;
      linesDrawn++;
      word = word.substring(cut);
    }

    if (linesDrawn < maxLines) {
      line = word;
      word = "";
    }
  };

  auto flushWord = [&]() {
    if (word.length() == 0 || linesDrawn >= maxLines) return;

    if (line.length() == 0) {
      placeWordOnEmptyLine();
      return;
    }

    String candidate = line + " " + word;
    if (tft.textWidth(candidate, font) <= maxW) {
      line = candidate;
      word = "";
      return;
    }

    flushLine();
    placeWordOnEmptyLine();
  };

  for (int i = 0; i < (int)text.length(); i++) {
    if (linesDrawn >= maxLines) break;
    char c = text[i];

    if (c == '\n') {
      flushWord();
      flushLine();
      continue;
    }

    if (c == ' ') {
      flushWord();
      continue;
    }

    word += c;
  }

  if (linesDrawn < maxLines) {
    flushWord();
    if (line.length() > 0) flushLine();
  }

  return y;
}

// Draws a widget value honoring the global font-size setting ("Widget-
// Schriftgröße"): small = font 2 (16 px), normal = font 4 (26 px), large =
// font 2 at 2x scale (32 px) at a fixed y so label (y=8, 16 px) + value stay
// inside the 58 px card. A large value too wide for the card falls back to
// the normal font for that widget instead of clipping at the sprite edge.
// Returns the drawn pixel width so callers can append a suffix behind the
// value.
int drawWidgetValueString(TFT_eSprite& spr, const String& value, int x, int yNormal, int cardW) {
  if (widgetFontKey == "small") {
    spr.drawString(value, x, yNormal, 2);
    return spr.textWidth(value, 2);
  }
  if (widgetFontKey == "large") {
    int scaledW = spr.textWidth(value, 2) * 2;
    if (scaledW <= cardW - x - 6) {
      spr.setTextSize(2);
      spr.drawString(value, x, 24, 2);
      spr.setTextSize(1);
      return scaledW;
    }
  }
  spr.drawString(value, x, yNormal, 4);
  return spr.textWidth(value, 4);
}

// =========================================================
// HOME SPRITES
// =========================================================
void drawClockCardSprite(bool force = false) {
  const int x = 8, y = 8, w = 224, h = 66;

  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);

  String timeBuf = formatClockParts(tmNow, true);
  String dateBuf = formatDateParts(tmNow);

  String sr = formatMinuteOfDay(sunriseMin);
  String ss = formatMinuteOfDay(sunsetMin);

  String combined = timeBuf + "|" + dateBuf + "|" + sr + "|" + ss + "|" +
                    String(COL_ACCENT) + "|" + String(COL_TEXT);

  if (!force && combined == cacheClock) return;
  cacheClock = combined;

  makeSpriteCard(sprClock, w, h, true);

  sprClock.setTextDatum(TL_DATUM);

  sprClock.setTextColor(COL_TEXT, COL_PANEL);
  if (useUsRegionFormat()) {
    int splitAt = timeBuf.lastIndexOf(' ');
    String clockMain = splitAt > 0 ? timeBuf.substring(0, splitAt) : timeBuf;
    String clockSuffix = splitAt > 0 ? timeBuf.substring(splitAt + 1) : "";
    sprClock.drawString(clockMain, 10, 11, 4);
    if (clockSuffix.length() > 0) {
      int suffixX = 10 + sprClock.textWidth(clockMain, 4) + 4;
      sprClock.drawString(clockSuffix, suffixX, 18, 2);
    }
  } else {
    sprClock.drawString(timeBuf, 10, 11, 4);
  }

  sprClock.setTextColor(COL_DIM, COL_PANEL);
  sprClock.drawString(dateBuf, 10, 45, 2);

  drawCleanSunIcon(sprClock, 151, 22, COL_ACCENT);
  drawMoonIcon(sprClock, 151, 50, COL_ACCENT);

  sprClock.setTextColor(COL_ACCENT, COL_PANEL);
  sprClock.drawString(sr, 165, 15, 2);
  sprClock.drawString(ss, 165, 43, 2);

  pushSpriteAndDelete(sprClock, x, y);
}

void drawMetricSprite(int x, int y, int w, int h, const char* label, const String& value, String& cache, bool force = false, const String& detail = "") {
  String combined = String(label) + "|" + value + "|" + detail + "|" + String(COL_PANEL) + "|" +
                    String(COL_STROKE) + "|" + String(COL_TEXT) + "|" + String(widgetBordersEnabled ? 1 : 0) + "|" + widgetFontKey;

  if (!force && combined == cache) return;
  cache = combined;

  makeSpriteCard(sprSmall, w, h, true);

  sprSmall.setTextDatum(TL_DATUM);
  sprSmall.setTextColor(COL_DIM, COL_PANEL);
  sprSmall.drawString(label, 10, 8, 2);

  sprSmall.setTextColor(COL_TEXT, COL_PANEL);
  drawWidgetValueString(sprSmall, value, 10, 31, w);

  if (detail.length() > 0) {
    sprSmall.setTextColor(COL_ACCENT, COL_PANEL);
    sprSmall.drawString(detail, 10, 55, 1);
  }

  pushSpriteAndDelete(sprSmall, x, y);
}

void drawWeatherStyleMetricSprite(int x, int y, int w, int h, const char* label, const String& value, String& cache, bool force = false, const String& detail = "") {
  String combined = String(label) + "|" + value + "|" + detail + "|" + String(COL_PANEL) + "|" +
                    String(COL_STROKE) + "|" + String(COL_TEXT) + "|" + String(widgetBordersEnabled ? 1 : 0) + "|" + widgetFontKey;

  if (!force && combined == cache) return;
  cache = combined;

  makeSpriteCard(sprSmall, w, h, true);

  sprSmall.setTextDatum(TL_DATUM);
  sprSmall.setTextColor(COL_DIM, COL_PANEL);
  sprSmall.drawString(label, 10, 8, 2);

  sprSmall.setTextColor(COL_TEXT, COL_PANEL);
  drawWidgetValueString(sprSmall, value, 10, 28, w);

  if (detail.length() > 0) {
    sprSmall.setTextColor(COL_ACCENT, COL_PANEL);
    sprSmall.drawString(detail, 10, 52, 1);
  }

  pushSpriteAndDelete(sprSmall, x, y);
}

void drawSunEventWidget(int x, int y, int w, int h, String& cache, bool force = false) {
  String label = nextSunLabel();
  String value = nextSunTimeText();
  String combined = label + "|" + value + "|" + String(COL_PANEL) + "|" +
                    String(COL_STROKE) + "|" + String(COL_TEXT) + "|" + String(COL_ACCENT) + "|" + String(widgetBordersEnabled ? 1 : 0) + "|" + widgetFontKey;

  if (!force && combined == cache) return;
  cache = combined;

  makeSpriteCard(sprSmall, w, h, true);

  sprSmall.setTextDatum(TL_DATUM);
  sprSmall.setTextColor(COL_DIM, COL_PANEL);
  sprSmall.drawString(label, 10, 8, 2);

  sprSmall.setTextColor(COL_TEXT, COL_PANEL);
  if (useUsRegionFormat()) {
    int splitAt = value.lastIndexOf(' ');
    String mainValue = splitAt > 0 ? value.substring(0, splitAt) : value;
    String suffix = splitAt > 0 ? value.substring(splitAt + 1) : "";
    // Reserve room for the am/pm suffix in the large-font fit check so the
    // suffix never clips at the card edge.
    int mainW = drawWidgetValueString(sprSmall, mainValue, 10, 30, suffix.length() > 0 ? w - 26 : w);
    if (suffix.length() > 0) {
      int suffixX = 10 + mainW + 3;
      sprSmall.drawString(suffix, suffixX, 35, 2);
    }
  } else {
    drawWidgetValueString(sprSmall, value, 10, 30, w);
  }

  pushSpriteAndDelete(sprSmall, x, y);
}

void drawPlaceholderSprite(int x, int y, int w, int h, const char* label, String& cache, bool force = false) {
  String combined = String(label) + "|" + String(COL_PANEL) + "|" + String(COL_STROKE) + "|" + String(COL_TEXT) + "|" + String(widgetBordersEnabled ? 1 : 0);

  if (!force && combined == cache) return;
  cache = combined;

  makeSpriteCard(sprSmall, w, h, true);

  sprSmall.setTextDatum(TL_DATUM);
  sprSmall.setTextColor(COL_DIM, COL_PANEL);
  sprSmall.drawString(label, 10, 8, 2);

  sprSmall.setTextColor(COL_STROKE, COL_PANEL);
  sprSmall.drawString(TXT(L_WIDGET_EMPTY), 10, 31, 2);

  pushSpriteAndDelete(sprSmall, x, y);
}

String mqttDisplayValue(int idx) {
  if (idx < 0 || idx >= MQTT_VALUE_COUNT) return "--";
  if (!mqttValueEnabled[idx]) return String(TXT(L_STATUS_DISABLED));
  String value = mqttFormattedValue(idx);
  String unit = mqttUnits[idx];
  unit.trim();
  if (unit.length() > 0 && value != "--" && value.length() > 0) value += " " + unit;
  return value;
}

static String dataPointDisplayValue(const DataPoint& point) {
  if (!point.valid || point.value.length() == 0) return "--";
  String value = point.value;
  String unit = point.unit;
  unit.trim();
  if (unit.length() > 0 && value != "--" && !value.endsWith(unit)) {
    value += " " + unit;
  }
  return value;
}

void drawWidgetByType(HomeWidgetType type, const String& dataPointId, int x, int y, int w, int h, String& cache, bool force = false) {
  switch (type) {
    case HOME_WIDGET_WEEK:
      drawMetricSprite(x, y, w, h, TXT(L_WIDGET_WEEK), weekNumberText(), cache, force);
      break;
    case HOME_WIDGET_RAIN:
      drawWeatherStyleMetricSprite(x, y, w, h, TXT(L_WIDGET_RAIN), rainText(), cache, force);
      break;
    case HOME_WIDGET_OUTDOOR:
      drawWeatherStyleMetricSprite(x, y, w, h, TXT(L_WIDGET_OUTDOOR), tempText(), cache, force);
      break;
    case HOME_WIDGET_KP:
      drawWeatherStyleMetricSprite(x, y, w, h, TXT(L_WIDGET_KP), kpText(), cache, force);
      break;
    case HOME_WIDGET_UV:
      drawWeatherStyleMetricSprite(x, y, w, h, TXT(L_WIDGET_UV), uvText(), cache, force);
      break;
    case HOME_WIDGET_WIND:
      drawWeatherStyleMetricSprite(x, y, w, h, TXT(L_WIDGET_WIND), windText(), cache, force);
      break;
    case HOME_WIDGET_SUN:
      drawSunEventWidget(x, y, w, h, cache, force);
      break;
    case HOME_WIDGET_MQTT_1:
    case HOME_WIDGET_MQTT_2:
    case HOME_WIDGET_MQTT_3:
    case HOME_WIDGET_MQTT_4:
    case HOME_WIDGET_MQTT_5:
    case HOME_WIDGET_MQTT_6:
    case HOME_WIDGET_MQTT_7:
    case HOME_WIDGET_MQTT_8:
    case HOME_WIDGET_MQTT_9:
    case HOME_WIDGET_MQTT_10:
    case HOME_WIDGET_MQTT_11:
    case HOME_WIDGET_MQTT_12: {
      int idx = (int)type - (int)HOME_WIDGET_MQTT_1;
      String value = mqttDisplayValue(idx);
      drawWeatherStyleMetricSprite(x, y, w, h, mqttLabels[idx].c_str(), value, cache, force);
      break;
    }
    case HOME_WIDGET_DATAPOINT: {
      const DataPoint* point = dataPointManager.get(dataPointId);
      if (!point) {
        drawPlaceholderSprite(x, y, w, h, "Datenpunkt", cache, force);
        break;
      }
      String value = dataPointDisplayValue(*point);
      drawWeatherStyleMetricSprite(x, y, w, h, point->label.c_str(), value, cache, force);
      break;
    }
  }
}

void drawHomeSlotWidget(int slot, bool force = false) {
  int x, y, w, h;
  getHomeSlotRect(slot, x, y, w, h);
  drawWidgetByType(homeWidgetSlots[slot], homeWidgetDataPointIds[slot], x, y, w, h, cacheHomeSlots[slot], force);
}

void drawCustomTabWidget(int tabIndex, int slot, bool force = false) {
  int x, y, w, h;
  getTabSlotRect(slot, x, y, w, h);
  drawWidgetByType(customTabWidgets[tabIndex][slot], customTabWidgetDataPointIds[tabIndex][slot], x, y, w, h, cacheCustomTabSlots[tabIndex][slot], force);
}
