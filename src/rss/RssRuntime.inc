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
// Text cleanup lives in include/InfoTermLogic.h (infoterm::rsstext, moved
// there for issue #5 so it is host-testable): entities decoded, tags
// stripped, UTF-8 transliterated to display-safe ASCII.
// ---------------------------------------------------------------------------

// Thin wrappers over the host-tested infoterm::rsstext pipeline
// (include/InfoTermLogic.h, extracted for issue #5): the firmware calls the
// exact code the native tests cover, not a copy.
static String rssCleanText(const String& s, int maxLen) {
  return infoterm::rsstext::cleanText<String>(s.c_str(), maxLen);
}

static String rssExtractTag(const String& xml, const char* tag) {
  return infoterm::rsstext::extractTag<String>(xml.c_str(), tag);
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
