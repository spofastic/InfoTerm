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

// Pure tab-rotation step for the burn-in cycle (1.0.12, issue #5 increment;
// extracted from nextCyclePage() while issue #9 raised the page count).
// `pages[0..count)` is the visible nav order; `skipPage` (the Info page) is
// never part of the rotation. Returns the page id after `current`; if
// `current` is not in the rotation (e.g. it IS the Info page), the cycle
// resumes at the first rotatable page. Fewer than two rotatable pages ->
// `current` unchanged (nothing to cycle).
inline int nextCyclePageId(const int* pages, int count, int skipPage, int current) {
  int cycleCount = 0;
  for (int i = 0; i < count; i++) {
    if (pages[i] != skipPage) cycleCount++;
  }
  if (cycleCount < 2) return current;

  int idx = -1;
  int pos = 0;
  for (int i = 0; i < count; i++) {
    if (pages[i] == skipPage) continue;
    if (pages[i] == current) { idx = pos; break; }
    pos++;
  }
  int target = (idx + 1) % cycleCount;
  pos = 0;
  for (int i = 0; i < count; i++) {
    if (pages[i] == skipPage) continue;
    if (pos == target) return pages[i];
    pos++;
  }
  return current;
}

// Host part of the URL: text between "scheme://" (optional) and the first
// '/'. Extracted from rssFeedOptionLabel() (1.0.12, issue #5 increment).
template <typename TString>
inline TString urlHost(const char* url) {
  const char* h = url;
  for (const char* q = url; *q; q++) {
    if (q[0] == ':' && q[1] == '/' && q[2] == '/') { h = q + 3; break; }
  }
  TString out;
  for (const char* q = h; *q && *q != '/'; q++) out += *q;
  return out;
}

// Gate for stored/fetched RSS feed URLs (1.1.0, security review): only
// plain http(s) targets without embedded credentials are allowed. The
// scheme must be literal lowercase "http://"/"https://" (matches the
// previous startsWith checks), the host part must be non-empty, and an
// '@' before the first path slash is rejected - user:pass@host URLs would
// leak credentials into logs and feed labels. Every path that stores feed
// URLs (form save, NVS load, backup/RSS import via sanitizeGeneralConfig)
// and the fetch itself funnel through this one predicate.
inline bool isAllowedFeedUrl(const char* url) {
  if (!url) return false;  // callers pass c_str(), never null - pure defense
  const char* host = nullptr;
  if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p') {
    if (url[4] == ':' && url[5] == '/' && url[6] == '/') host = url + 7;
    else if (url[4] == 's' && url[5] == ':' && url[6] == '/' && url[7] == '/') host = url + 8;
  }
  if (!host || *host == '\0' || *host == '/') return false;
  for (const char* q = host; *q && *q != '/'; q++) {
    if (*q == '@') return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// RSS text pipeline (1.0.11, issue #5 increment): extracted from
// src/rss/RssRuntime.inc so the parsing/cleanup logic - the newest pure
// string code in the firmware - is host-testable. Templated over the string
// type like htmlEscaped above; the only required TString operations are
// `+=` (char and C-string) and construction, so Arduino String and
// std::string both work. Inputs are plain `const char*`.
//
// Pipeline background: RSS/Atom content arrives as UTF-8 HTML with
// entities; the TFT fonts (2/4) only cover ASCII, so everything is
// transliterated to a display-safe ASCII form (ae/oe/ue/ss, accents
// stripped, typographic punctuation mapped to its ASCII cousin, all other
// codepoints dropped).
// ---------------------------------------------------------------------------
namespace rsstext {

template <typename TString>
inline void appendCodepointAscii(TString& out, uint32_t cp) {
  if (cp == 0x00A0) { out += ' '; return; }              // nbsp
  if (cp < 128) {
    char c = (char)cp;
    if (c == '\t') c = ' ';
    if (c >= 32 || c == '\n') out += c;
    return;
  }
  switch (cp) {
    case 0xC4: out += "Ae"; break;
    case 0xD6: out += "Oe"; break;
    case 0xDC: out += "Ue"; break;
    case 0xE4: out += "ae"; break;
    case 0xF6: out += "oe"; break;
    case 0xFC: out += "ue"; break;
    case 0xDF: out += "ss"; break;
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: out += 'A'; break;
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: out += 'E'; break;
    case 0xCC: case 0xCD: case 0xCE: case 0xCF: out += 'I'; break;
    case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD8: out += 'O'; break;
    case 0xD9: case 0xDA: case 0xDB: out += 'U'; break;
    case 0xC7: out += 'C'; break;
    case 0xD1: out += 'N'; break;
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE5: out += 'a'; break;
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: out += 'e'; break;
    case 0xEC: case 0xED: case 0xEE: case 0xEF: out += 'i'; break;
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF8: out += 'o'; break;
    case 0xF9: case 0xFA: case 0xFB: out += 'u'; break;
    case 0xE7: out += 'c'; break;
    case 0xF1: out += 'n'; break;
    case 0xFD: case 0xFF: out += 'y'; break;
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: out += '-'; break;
    case 0x2018: case 0x2019: case 0x201A: out += '\''; break;
    case 0x201C: case 0x201D: case 0x201E: out += '"'; break;
    case 0x2026: out += "..."; break;
    case 0x20AC: out += "EUR"; break;
    default: break;  // no ASCII stand-in: drop silently
  }
}

template <typename TString>
inline TString utf8ToAscii(const char* in) {
  TString out;
  const unsigned char* p = (const unsigned char*)in;
  while (*p) {
    unsigned char b = *p;
    uint32_t cp = 0;
    int extra = 0;
    if (b < 0x80) { cp = b; }
    else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; extra = 1; }
    else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; extra = 2; }
    else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; extra = 3; }
    else { p++; continue; }  // stray continuation byte
    bool valid = true;
    for (int k = 1; k <= extra; k++) {
      unsigned char cb = p[k];
      if ((cb & 0xC0) != 0x80) { valid = false; break; }  // also catches NUL
      cp = (cp << 6) | (cb & 0x3F);
    }
    if (!valid) { p++; continue; }
    appendCodepointAscii(out, cp);
    p += 1 + extra;
  }
  return out;
}

// Removes <...> tags (CDATA markers dropped, their content kept) and turns
// paragraph/line-break closers into '\n' so the display keeps structure.
template <typename TString>
inline TString stripTags(const char* in) {
  TString out;
  const char* p = in;
  auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; };
  while (*p) {
    if (p[0] == '<' && p[1] == '!' && p[2] == '[' && p[3] == 'C' && p[4] == 'D' &&
        p[5] == 'A' && p[6] == 'T' && p[7] == 'A' && p[8] == '[') { p += 9; continue; }
    if (p[0] == ']' && p[1] == ']' && p[2] == '>') { p += 3; continue; }
    if (*p == '<') {
      const char* gt = p + 1;
      while (*gt && *gt != '>') gt++;
      if (!*gt) break;  // unterminated tag: drop the tail
      const char* t = p + 1;
      bool nl = false;
      if (lower(t[0]) == 'b' && lower(t[1]) == 'r') nl = true;
      else if (t[0] == '/') {
        char c1 = lower(t[1]);
        if (c1 == 'p' || c1 == 'h') nl = true;
        else if (c1 == 'd' && lower(t[2]) == 'i' && lower(t[3]) == 'v') nl = true;
        else if (c1 == 'l' && lower(t[2]) == 'i') nl = true;
      }
      if (nl) out += '\n';
      p = gt + 1;
      continue;
    }
    out += *p++;
  }
  return out;
}

template <typename TString>
inline TString decodeEntities(const char* in) {
  TString out;
  const char* p = in;
  while (*p) {
    if (*p != '&') { out += *p++; continue; }
    const char* semi = p + 1;
    while (*semi && *semi != ';' && semi - p <= 10) semi++;
    if (*semi != ';') { out += *p++; continue; }
    char ent[12];
    int elen = (int)(semi - (p + 1));
    for (int k = 0; k < elen; k++) ent[k] = p[1 + k];
    ent[elen] = 0;
    if (ent[0] == 'a' && ent[1] == 'm' && ent[2] == 'p' && elen == 3) out += '&';
    else if (ent[0] == 'l' && ent[1] == 't' && elen == 2) out += '<';
    else if (ent[0] == 'g' && ent[1] == 't' && elen == 2) out += '>';
    else if (elen == 4 && ent[0] == 'q' && ent[1] == 'u' && ent[2] == 'o' && ent[3] == 't') out += '"';
    else if (elen == 4 && ent[0] == 'a' && ent[1] == 'p' && ent[2] == 'o' && ent[3] == 's') out += '\'';
    else if (elen == 4 && ent[0] == 'n' && ent[1] == 'b' && ent[2] == 's' && ent[3] == 'p') out += ' ';
    else if (ent[0] == '#') {
      long cp = 0;
      if (ent[1] == 'x' || ent[1] == 'X') {
        for (int k = 2; k < elen; k++) {
          char c = ent[k];
          int d;
          if (c >= '0' && c <= '9') d = c - '0';
          else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
          else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
          else { cp = 0; break; }
          cp = cp * 16 + d;
        }
      } else {
        for (int k = 1; k < elen; k++) {
          if (ent[k] < '0' || ent[k] > '9') { cp = 0; break; }
          cp = cp * 10 + (ent[k] - '0');
        }
      }
      if (cp > 0) appendCodepointAscii(out, (uint32_t)cp);
    } else {
      out += '&';
      out += ent;
      out += ';';
    }
    p = semi + 1;
  }
  return out;
}

// First <tag>...</tag> content in xml; skips self-closing tags and requires
// a real tag-name boundary so "title" never matches inside e.g. "subtitle".
// Returns raw inner markup (run it through cleanText for display).
template <typename TString>
inline TString extractTag(const char* xml, const char* tag) {
  int tlen = 0;
  while (tag[tlen]) tlen++;
  const char* from = xml;
  while (*from) {
    // find next "<" + tag
    const char* p = from;
    while (*p) {
      if (*p == '<') {
        int k = 0;
        while (k < tlen && p[1 + k] == tag[k]) k++;
        if (k == tlen) break;
      }
      p++;
    }
    if (!*p) return TString();
    char after = p[1 + tlen];
    if (after != '>' && after != ' ' && after != '\t' && after != '\r' && after != '\n') {
      from = p + 1;
      continue;
    }
    const char* gt = p;
    while (*gt && *gt != '>') gt++;
    if (!*gt) return TString();
    if (gt[-1] == '/') { from = gt + 1; continue; }  // self-closing
    // find "</" + tag after gt
    const char* e = gt + 1;
    while (*e) {
      if (e[0] == '<' && e[1] == '/') {
        int k = 0;
        while (k < tlen && e[2 + k] == tag[k]) k++;
        if (k == tlen) break;
      }
      e++;
    }
    if (!*e) return TString();
    TString out;
    for (const char* c = gt + 1; c < e; c++) out += *c;
    return out;
  }
  return TString();
}

// Full cleanup pipeline for one extracted XML field. Entities can encode
// literal markup (&lt;p&gt;), so tags are stripped again after decoding.
// Collapses whitespace runs (at most one '\n' in a row, no leading/trailing
// whitespace) and truncates to maxLen visible chars plus "...".
template <typename TString>
inline TString cleanText(const char* in, int maxLen) {
  TString a = stripTags<TString>(in);
  {
    TString b = decodeEntities<TString>(a.c_str());
    TString c = stripTags<TString>(b.c_str());
    a = utf8ToAscii<TString>(c.c_str());
  }
  TString out;
  int outLen = 0;
  bool pendingSpace = false;
  bool pendingNewline = false;
  bool truncated = false;
  for (const char* p = a.c_str(); *p; ++p) {
    char c = *p;
    if (c == '\r') continue;
    if (c == '\n') { if (outLen > 0) pendingNewline = true; continue; }
    if (c == ' ' || c == '\t') { if (outLen > 0) pendingSpace = true; continue; }
    if (outLen >= maxLen) { truncated = true; break; }
    if (pendingNewline) { out += '\n'; outLen++; pendingNewline = false; pendingSpace = false; }
    else if (pendingSpace) { out += ' '; outLen++; pendingSpace = false; }
    if (outLen >= maxLen) { truncated = true; break; }
    out += c;
    outLen++;
  }
  if (truncated) out += "...";
  return out;
}

}  // namespace rsstext

}  // namespace infoterm
