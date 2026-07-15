/******************************************************************************
 * InfoTerm
 * Module  : RSS feed runtime (1.0.10, issue #7)
 *
 * Fetches the newest entry of up to RSS_FEED_COUNT configured feeds
 * (RSS 2.0 <item> or Atom <entry>) and renders it on custom tabs whose
 * content selector is set to "rss". Long entries scroll vertically.
 *
 * Fetching reuses the established blocking-HTTP pattern from
 * WeatherRuntime.inc (WiFiClientSecure + setInsecure + HTTPClient); the
 * scheduler fetches at most ONE feed per tick to bound how long a single
 * cooperative loop iteration can block. The body is read in a streaming
 * loop capped at RSS_READ_LIMIT bytes and stops as soon as the first
 * complete item has arrived, so oversized feeds cannot exhaust RAM.
 ******************************************************************************/

// ---------------------------------------------------------------------------
// Text cleanup: RSS content arrives as UTF-8 HTML with entities; the TFT
// fonts (2/4) only cover ASCII, so everything is transliterated to a
// display-safe ASCII form (ae/oe/ue/ss, accents stripped, typographic
// punctuation mapped to its ASCII cousin, all other codepoints dropped).
// ---------------------------------------------------------------------------

static void rssAppendCodepointAscii(String& out, uint32_t cp) {
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

static String rssUtf8ToAscii(const String& in) {
  String out;
  out.reserve(in.length());
  int i = 0;
  int len = in.length();
  while (i < len) {
    uint8_t b = (uint8_t)in[i];
    uint32_t cp = 0;
    int extra = 0;
    if (b < 0x80) { cp = b; }
    else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; extra = 1; }
    else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; extra = 2; }
    else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; extra = 3; }
    else { i++; continue; }  // stray continuation byte
    if (i + extra >= len) break;  // truncated multibyte sequence at the end
    bool valid = true;
    for (int k = 1; k <= extra; k++) {
      uint8_t cb = (uint8_t)in[i + k];
      if ((cb & 0xC0) != 0x80) { valid = false; break; }
      cp = (cp << 6) | (cb & 0x3F);
    }
    if (!valid) { i++; continue; }
    rssAppendCodepointAscii(out, cp);
    i += 1 + extra;
  }
  return out;
}

// Removes <...> tags (CDATA markers first) and turns paragraph/line breaks
// into '\n' so the display keeps some structure.
static String rssStripTags(const String& in) {
  String s = in;
  s.replace("<![CDATA[", "");
  s.replace("]]>", "");
  String out;
  out.reserve(s.length());
  int i = 0;
  int len = s.length();
  while (i < len) {
    char c = s[i];
    if (c == '<') {
      int gt = s.indexOf('>', i);
      if (gt < 0) break;
      String tag = s.substring(i + 1, gt);
      tag.toLowerCase();
      if (tag.startsWith("br") || tag.startsWith("/p") || tag.startsWith("/div") ||
          tag.startsWith("/li") || tag.startsWith("/h")) {
        out += '\n';
      }
      i = gt + 1;
      continue;
    }
    out += c;
    i++;
  }
  return out;
}

static String rssDecodeEntities(const String& in) {
  String out;
  out.reserve(in.length());
  int i = 0;
  int len = in.length();
  while (i < len) {
    char c = in[i];
    if (c != '&') { out += c; i++; continue; }
    int semi = in.indexOf(';', i);
    if (semi < 0 || semi - i > 10) { out += c; i++; continue; }
    String ent = in.substring(i + 1, semi);
    if (ent == "amp") out += '&';
    else if (ent == "lt") out += '<';
    else if (ent == "gt") out += '>';
    else if (ent == "quot") out += '"';
    else if (ent == "apos") out += '\'';
    else if (ent == "nbsp") out += ' ';
    else if (ent.startsWith("#")) {
      long cp = ent.startsWith("#x") || ent.startsWith("#X")
                    ? strtol(ent.c_str() + 2, nullptr, 16)
                    : strtol(ent.c_str() + 1, nullptr, 10);
      if (cp > 0) rssAppendCodepointAscii(out, (uint32_t)cp);
    } else {
      out += '&';
      out += ent;
      out += ';';
    }
    i = semi + 1;
  }
  return out;
}

// Full cleanup pipeline for one extracted XML field. Entities can encode
// literal markup (&lt;p&gt;), so tags are stripped again after decoding.
static String rssCleanText(String s, int maxLen) {
  s = rssStripTags(s);
  s = rssDecodeEntities(s);
  s = rssStripTags(s);
  s = rssUtf8ToAscii(s);
  // Collapse whitespace runs, keep at most one newline in a row.
  String out;
  out.reserve(s.length());
  bool lastSpace = true;
  bool lastNewline = false;
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '\r') continue;
    if (c == '\n') {
      if (!lastNewline && out.length() > 0) out += '\n';
      lastSpace = true;
      lastNewline = true;
      continue;
    }
    if (c == ' ') {
      if (!lastSpace) out += ' ';
      lastSpace = true;
      continue;
    }
    out += c;
    lastSpace = false;
    lastNewline = false;
  }
  out.trim();
  if ((int)out.length() > maxLen) {
    out = out.substring(0, maxLen);
    out += "...";
  }
  return out;
}

// First <tag>...</tag> content in xml; skips self-closing tags and requires
// a real tag-name boundary so "title" never matches inside e.g. "subtitle".
static String rssExtractTag(const String& xml, const char* tag) {
  String open = String("<") + tag;
  String close = String("</") + tag;
  int from = 0;
  while (true) {
    int p = xml.indexOf(open, from);
    if (p < 0) return "";
    int afterIdx = p + open.length();
    if (afterIdx >= (int)xml.length()) return "";
    char after = xml[afterIdx];
    if (after != '>' && after != ' ' && after != '\t' && after != '\r' && after != '\n') {
      from = p + 1;
      continue;
    }
    int gt = xml.indexOf('>', p);
    if (gt < 0) return "";
    if (xml[gt - 1] == '/') { from = gt + 1; continue; }  // self-closing
    int e = xml.indexOf(close, gt + 1);
    if (e < 0) return "";
    return xml.substring(gt + 1, e);
  }
}

// Short label for a feed in WebGUI dropdowns: fetched channel title if
// known, otherwise the URL host.
String rssFeedOptionLabel(int idx) {
  if (idx < 0 || idx >= RSS_FEED_COUNT) return "";
  if (rssChannelTitle[idx].length() > 0) return rssChannelTitle[idx];
  String url = rssFeedUrls[idx];
  int scheme = url.indexOf("://");
  if (scheme >= 0) url = url.substring(scheme + 3);
  int slash = url.indexOf('/');
  if (slash > 0) url = url.substring(0, slash);
  if (url.length() == 0) url = String("Feed ") + String(idx + 1);
  return url;
}

void rssResetFeedState(int idx) {
  if (idx < 0 || idx >= RSS_FEED_COUNT) return;
  rssChannelTitle[idx] = "";
  rssEntryTitle[idx] = "";
  rssEntryText[idx] = "";
  rssEntryDate[idx] = "";
  rssFetchOk[idx] = false;
  rssFetchTried[idx] = false;
  rssLastFetchMs[idx] = 0;
}

// Is this feed shown by any enabled RSS tab? Only referenced feeds are
// fetched at all.
static bool rssFeedReferenced(int idx) {
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    if (customTabEnabled[t] && customTabContent[t] == "rss" && customTabFeedIndex[t] == idx) return true;
  }
  return false;
}

bool fetchRssFeed(int idx) {
  if (idx < 0 || idx >= rssFeedCount) return false;
  rssFetchTried[idx] = true;
  rssLastFetchMs[idx] = millis();
  rssFetchOk[idx] = false;

  String url = rssFeedUrls[idx];
  url.trim();
  if (url.length() == 0 || WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure secureClient;
  WiFiClient plainClient;
  HTTPClient http;
  bool began;
  if (url.startsWith("https://")) {
    secureClient.setInsecure();
    began = http.begin(secureClient, url);
  } else {
    began = http.begin(plainClient, url);
  }
  if (!began) return false;

  http.setTimeout(8000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("InfoTerm/" INFOTERM_APP_VERSION);
  int code = http.GET();
  if (code != 200) {
    http.end();
    Logger::info(String("[RSS] feed ") + String(idx) + " HTTP " + String(code));
    return false;
  }

  // Stream the body: stop after RSS_READ_LIMIT bytes or as soon as the
  // first complete item/entry is in the buffer - only the newest entry is
  // ever displayed, the rest of the document is irrelevant.
  const size_t RSS_READ_LIMIT = 24 * 1024;
  WiFiClient* stream = http.getStreamPtr();
  String body;
  body.reserve(4096);
  unsigned long deadline = millis() + 8000;
  while (millis() < deadline && body.length() < RSS_READ_LIMIT) {
    size_t avail = stream ? stream->available() : 0;
    if (avail > 0) {
      char buf[512];
      size_t toRead = avail > sizeof(buf) ? sizeof(buf) : avail;
      int n = stream->readBytes(buf, toRead);
      if (n <= 0) break;
      body.concat(buf, n);
      if (body.indexOf("</item>") >= 0 || body.indexOf("</entry>") >= 0) break;
    } else if (!http.connected()) {
      break;
    } else {
      delay(1);
    }
  }
  http.end();

  bool atom = false;
  int itemPos = body.indexOf("<item>");
  if (itemPos < 0) itemPos = body.indexOf("<item ");
  if (itemPos < 0) {
    itemPos = body.indexOf("<entry>");
    if (itemPos < 0) itemPos = body.indexOf("<entry ");
    atom = itemPos >= 0;
  }

  String head = itemPos >= 0 ? body.substring(0, itemPos) : body;
  String channel = rssCleanText(rssExtractTag(head, "title"), 60);
  if (channel.length() > 0) rssChannelTitle[idx] = channel;

  if (itemPos < 0) {
    Logger::info(String("[RSS] feed ") + String(idx) + " has no item/entry");
    return false;
  }

  String item = body.substring(itemPos);
  int endPos = item.indexOf(atom ? "</entry>" : "</item>");
  if (endPos > 0) item = item.substring(0, endPos);

  String title = rssExtractTag(item, "title");
  String text = rssExtractTag(item, "description");
  if (text.length() == 0) text = rssExtractTag(item, "summary");
  if (text.length() == 0) text = rssExtractTag(item, "content:encoded");
  if (text.length() == 0 && atom) text = rssExtractTag(item, "content");
  String date = rssExtractTag(item, "pubDate");
  if (date.length() == 0) date = rssExtractTag(item, "updated");
  if (date.length() == 0) date = rssExtractTag(item, "dc:date");

  rssEntryTitle[idx] = rssCleanText(title, 200);
  rssEntryText[idx] = rssCleanText(text, 600);
  rssEntryDate[idx] = rssCleanText(date, 40);
  rssFetchOk[idx] = rssEntryTitle[idx].length() > 0 || rssEntryText[idx].length() > 0;
  Logger::info(String("[RSS] feed ") + String(idx) + (rssFetchOk[idx] ? " updated: " : " empty: ") + rssEntryTitle[idx]);
  return rssFetchOk[idx];
}

// Scheduler entry (Application.inc, 60 s tick): fetch at most one due,
// referenced feed per tick so a slow server blocks the loop only once.
void rssFetchDueFeeds() {
  if (portalActive || WiFi.status() != WL_CONNECTED) return;
  for (int i = 0; i < rssFeedCount; i++) {
    if (!rssFeedReferenced(i)) continue;
    if (rssFeedUrls[i].length() == 0) continue;
    if (rssFetchTried[i] && millis() - rssLastFetchMs[i] < RSS_FETCH_INTERVAL_MS) continue;
    fetchRssFeed(i);
    break;
  }
}

// ---------------------------------------------------------------------------
// Display rendering. The entry is wrapped into rssLines[] once and drawn
// through a viewport sprite; when the text is taller than the viewport it
// scrolls up pixel by pixel (serviceRssScroll, 100 ms scheduler task) with a
// pause at the top and bottom.
// ---------------------------------------------------------------------------

static const int RSS_CARD_X = 8;
static const int RSS_CARD_Y = 6;
static const int RSS_CARD_W = 224;   // SCREEN_W - 2*8
static const int RSS_VIEW_X = 16;
static const int RSS_VIEW_Y = 14;
static const int RSS_VIEW_W = 208;
static const int RSS_MAX_LINES = 56;
static const unsigned long RSS_SCROLL_PAUSE_MS = 2500;

static String rssLines[RSS_MAX_LINES];
static uint8_t rssLineColorClass[RSS_MAX_LINES];  // 0 = text, 1 = dim, 2 = accent
static int rssLineCount = 0;
static int rssScrollOffset = 0;
static bool rssScrollAtEnd = false;
static unsigned long rssScrollPauseUntil = 0;
static String rssShownSignature = "";

static int rssCardHeight() { return SCREEN_H - NAV_H - 2 * RSS_CARD_Y; }
static int rssViewHeight() { return rssCardHeight() - 2 * (RSS_VIEW_Y - RSS_CARD_Y); }

static int rssLineHeightPx() { return tft.fontHeight(2) + 2; }

static uint16_t rssLineColorOf(int i) {
  switch (rssLineColorClass[i]) {
    case 1: return COL_DIM;
    case 2: return COL_ACCENT;
    default: return COL_TEXT;
  }
}

static void rssAddLine(const String& line, uint8_t colorClass) {
  if (rssLineCount >= RSS_MAX_LINES) return;
  rssLines[rssLineCount] = line;
  rssLineColorClass[rssLineCount] = colorClass;
  rssLineCount++;
}

// Word-wraps text into rssLines (same approach as drawWrappedTextLimited in
// WidgetRuntime.inc, but collecting lines instead of drawing them).
static void rssAddWrapped(const String& text, uint8_t colorClass) {
  String line = "";
  String word = "";

  auto flushLine = [&]() {
    if (line.length() > 0) rssAddLine(line, colorClass);
    line = "";
  };

  auto flushWord = [&]() {
    if (word.length() == 0) return;
    // Hard-split words wider than the viewport.
    while (tft.textWidth(word, 2) > RSS_VIEW_W && word.length() > 1) {
      if (line.length() > 0) flushLine();
      int cut = word.length();
      while (cut > 1 && tft.textWidth(word.substring(0, cut), 2) > RSS_VIEW_W) cut--;
      rssAddLine(word.substring(0, cut), colorClass);
      word = word.substring(cut);
    }
    if (line.length() == 0) {
      line = word;
    } else {
      String candidate = line + " " + word;
      if (tft.textWidth(candidate, 2) <= RSS_VIEW_W) {
        line = candidate;
      } else {
        flushLine();
        line = word;
      }
    }
    word = "";
  };

  for (int i = 0; i < (int)text.length(); i++) {
    if (rssLineCount >= RSS_MAX_LINES) return;
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
  flushWord();
  flushLine();
}

// Status/content signature of the tab's feed - a change means the lines
// must be rebuilt and the viewport repainted.
static String rssTabSignature(int tabIndex) {
  int f = customTabFeedIndex[tabIndex];
  if (rssFeedCount == 0 || f < 0 || f >= rssFeedCount || rssFeedUrls[f].length() == 0) return "nofeed";
  if (!rssFetchTried[f]) return "loading";
  if (!rssFetchOk[f]) return "error|" + rssFeedUrls[f];
  return rssChannelTitle[f] + "|" + rssEntryDate[f] + "|" + rssEntryTitle[f] + "|" + rssEntryText[f];
}

static void rssBuildLines(int tabIndex) {
  rssLineCount = 0;
  String sig = rssTabSignature(tabIndex);
  int f = customTabFeedIndex[tabIndex];

  if (sig == "nofeed") {
    rssAddWrapped(TXT(L_RSS_NO_FEED), 1);
  } else if (sig == "loading") {
    rssAddWrapped(TXT(L_RSS_LOADING), 1);
  } else if (sig.startsWith("error|")) {
    rssAddWrapped(TXT(L_RSS_ERROR), 2);
    rssAddWrapped(rssFeedOptionLabel(f), 1);
  } else {
    if (rssChannelTitle[f].length() > 0) rssAddWrapped(rssChannelTitle[f], 1);
    if (rssEntryDate[f].length() > 0) rssAddWrapped(rssEntryDate[f], 1);
    if (rssLineCount > 0) rssAddLine("", 0);
    if (rssEntryTitle[f].length() > 0) {
      rssAddWrapped(rssEntryTitle[f], 2);
      rssAddLine("", 0);
    }
    if (rssEntryText[f].length() > 0) rssAddWrapped(rssEntryText[f], 0);
  }
  rssShownSignature = sig;
}

// Repaints the visible part of the entry. An 8-bit sprite keeps the
// full-viewport repaint flicker-free (same reasoning as the widget sprites)
// at ~1/4 of the 16-bit RAM cost; if allocation ever fails, fall back to a
// static direct draw without scrolling.
static void rssRenderViewport() {
  int viewH = rssViewHeight();
  int lineH = rssLineHeightPx();
  TFT_eSprite spr(&tft);
  spr.setColorDepth(8);
  spr.createSprite(RSS_VIEW_W, viewH);
  if (!spr.created()) {
    tft.fillRect(RSS_VIEW_X, RSS_VIEW_Y, RSS_VIEW_W, viewH, COL_PANEL);
    tft.setTextDatum(TL_DATUM);
    int maxLines = viewH / lineH;
    for (int i = 0; i < rssLineCount && i < maxLines; i++) {
      tft.setTextColor(rssLineColorOf(i), COL_PANEL);
      tft.drawString(rssLines[i], RSS_VIEW_X, RSS_VIEW_Y + i * lineH, 2);
    }
    return;
  }
  spr.fillSprite(COL_PANEL);
  spr.setTextDatum(TL_DATUM);
  int first = rssScrollOffset / lineH;
  for (int i = first; i < rssLineCount; i++) {
    int y = i * lineH - rssScrollOffset;
    if (y >= viewH) break;
    if (rssLines[i].length() == 0) continue;
    spr.setTextColor(rssLineColorOf(i), COL_PANEL);
    spr.drawString(rssLines[i], 0, y, 2);
  }
  spr.pushSprite(RSS_VIEW_X, RSS_VIEW_Y);
  spr.deleteSprite();
}

void drawRssTabPageFull(int tabIndex) {
  tft.fillScreen(COL_BG);
  drawNavBar();

  pageDirty = false;
  lastDrawnPage = pageForCustomTab(tabIndex);

  drawCard(RSS_CARD_X, RSS_CARD_Y, RSS_CARD_W, rssCardHeight(), true);
  rssBuildLines(tabIndex);
  rssScrollOffset = 0;
  rssScrollAtEnd = false;
  rssScrollPauseUntil = millis() + RSS_SCROLL_PAUSE_MS;
  rssRenderViewport();
}

// 1 Hz dynamic refresh: repaint only when the feed content/status actually
// changed (a background fetch finished). Scroll movement runs separately in
// serviceRssScroll so an unchanged entry never repaints at 1 Hz.
void updateRssTabDynamic(int tabIndex) {
  if (rssTabSignature(tabIndex) == rssShownSignature) return;
  rssBuildLines(tabIndex);
  rssScrollOffset = 0;
  rssScrollAtEnd = false;
  rssScrollPauseUntil = millis() + RSS_SCROLL_PAUSE_MS;
  rssRenderViewport();
}

// Scheduler task (100 ms): advances the vertical scroll by one pixel while
// an RSS tab with overflowing content is visible. Pauses at both ends, then
// wraps back to the top.
void serviceRssScroll() {
  if (portalActive || sleepDimmed || sleepOff || pageDirty) return;
  int tabIndex = -1;
  if (currentPage == PAGE_TAB1) tabIndex = 0;
  else if (currentPage == PAGE_TAB2) tabIndex = 1;
  else if (currentPage == PAGE_TAB3) tabIndex = 2;
  if (tabIndex < 0 || customTabContent[tabIndex] != "rss") return;
  if (lastDrawnPage != currentPage) return;  // full draw still pending

  int viewH = rssViewHeight();
  int total = rssLineCount * rssLineHeightPx();
  if (total <= viewH) return;

  unsigned long now = millis();
  if (now < rssScrollPauseUntil) return;

  if (rssScrollAtEnd) {
    rssScrollOffset = 0;
    rssScrollAtEnd = false;
    rssScrollPauseUntil = now + RSS_SCROLL_PAUSE_MS;
  } else {
    rssScrollOffset++;
    if (rssScrollOffset >= total - viewH) {
      rssScrollOffset = total - viewH;
      rssScrollAtEnd = true;
      rssScrollPauseUntil = now + RSS_SCROLL_PAUSE_MS;
    }
  }
  rssRenderViewport();
}
