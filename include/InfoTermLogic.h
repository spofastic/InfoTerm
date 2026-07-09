/******************************************************************************
 * InfoTerm - pure, dependency-free logic
 *
 * Deliberately free of any Arduino/ESP32 headers so it can be compiled and
 * unit-tested on a host (see test/ and the [env:native] target in
 * platformio.ini). The firmware includes this header and calls these
 * functions, so the host tests cover the exact code that ships - not a copy.
 ******************************************************************************/
#pragma once

#include <cstdint>

namespace infoterm {

inline int clampInt(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// Blend an RGB565 color toward an 8-bit target color by factor t (0..1).
// Exact port of the former static mixRgb565() in InfoTerm.ino - the whole
// theme/night-mode palette is derived through this, so the math must stay
// bit-for-bit identical.
inline uint16_t mixRgb565(uint16_t base565, uint8_t towardR, uint8_t towardG,
                          uint8_t towardB, float t) {
  uint8_t r = ((base565 >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((base565 >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (base565 & 0x1F) * 255 / 31;
  uint8_t nr = (uint8_t)clampInt((int)(r * (1.0f - t) + towardR * t), 0, 255);
  uint8_t ng = (uint8_t)clampInt((int)(g * (1.0f - t) + towardG * t), 0, 255);
  uint8_t nb = (uint8_t)clampInt((int)(b * (1.0f - t) + towardB * t), 0, 255);
  return ((nr * 31 / 255) << 11) | ((ng * 63 / 255) << 5) | (nb * 31 / 255);
}

// Night-mode window predicate. All values are minutes-of-day; sunriseMin or
// sunsetMin < 0 means "not known yet" (no weather fetch since boot) and is
// treated as daytime. Sunrise is exclusive (05:16 sharp = day), sunset is
// inclusive (21:25 sharp = night).
inline bool isNightWindow(int nowMin, int sunriseMin, int sunsetMin) {
  if (sunriseMin < 0 || sunsetMin < 0) return false;
  return nowMin < sunriseMin || nowMin >= sunsetMin;
}

// HTML-escapes raw text for embedding in HTML bodies and in single- or
// double-quoted attribute values. The single quote MUST stay covered
// (stored-XSS review finding, July 2026): the WebGUI renders stored
// configuration values into single-quoted attributes (value='...',
// data-selected='...', data-label='...'), so an apostrophe in a stored
// value would otherwise break out of the attribute and inject markup.
// Templated over the string type so the firmware (Arduino String) and the
// host tests (std::string) exercise this exact implementation.
template <typename TString>
inline TString htmlEscaped(const char* raw) {
  TString out;
  for (const char* p = raw; *p; ++p) {
    switch (*p) {
      case '&':  out += "&amp;";  break;
      case '<':  out += "&lt;";   break;
      case '>':  out += "&gt;";   break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;";  break;
      default:   out += *p;       break;
    }
  }
  return out;
}

}  // namespace infoterm
