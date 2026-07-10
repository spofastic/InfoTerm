/*
 * InfoTerm WebGUI Runtime
 * Version : 0.6.5
 *
 * Migrated from InfoTerm.ino in version 0.6.5. Contains the HTTP handlers,
 * WebGUI page rendering and MQTT DataPoint form actions. Kept as an .inc
 * runtime module so existing global runtime state remains unchanged during
 * the migration step.
 */

#include <Update.h>
#include "system/EventBusIntegration.h"
#include "config/BackupRestoreManager.h"

// Defined in wifi/WifiRuntime.inc, included later in the InfoTerm.cpp unity
// build - forward-declared here so handleSave() can trigger an immediate
// reconnect when the preferred network selection changes.
void beginWiFiConnect();

// Defined in vpn/VpnRuntime.inc, included earlier than this file - listed
// here too for readability at the call sites below.
bool vpnParseConfigFile(const char* path);
void vpnRequestConnect();
void vpnDisconnect();
String vpnStatusText();

// =========================================================
// WEB SERVER
// =========================================================

// HTTP Basic auth gate for mutating/sensitive routes (see Config.h for the
// route list and WEBGUI_USER/WEBGUI_PASS). Read-only pages (/, /datapoints)
// stay open so the dashboard remains glanceable.
//
// webAuthOk() only checks (no 401 challenge) - used inside multipart upload
// callbacks to decide whether to accept streamed bytes. requireAuth() checks
// and, on failure, sends the 401 challenge and returns false so the caller
// can bail out. An empty WEBGUI_PASS disables the gate entirely.
static bool webAuthOk() {
  if (strlen(WEBGUI_PASS) == 0) return true;
  return server.authenticate(WEBGUI_USER, WEBGUI_PASS);
}

static bool requireAuth() {
  if (webAuthOk()) return true;
  server.requestAuthentication();
  return false;
}

// Per-boot CSRF token. Basic Auth alone does not stop cross-site requests:
// the browser attaches cached credentials to any request a foreign page
// triggers against the device. Mutating routes therefore additionally
// require this token, which only pages rendered by the device itself carry
// (a cross-origin page cannot read it). For the multipart uploads the token
// travels in the URL query string, because query args are already parsed
// when the upload callback streams the first bytes - checking only in the
// done-handler would let a forged OTA upload write (and finalize) the
// firmware before being rejected.
static String webCsrfToken;

static void ensureCsrfToken() {
  if (webCsrfToken.length() > 0) return;
  char buf[17];
  snprintf(buf, sizeof(buf), "%08lx%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random());
  webCsrfToken = buf;
}

static bool csrfOk() {
  return webCsrfToken.length() > 0 && server.hasArg("csrf") &&
         server.arg("csrf") == webCsrfToken;
}

static bool requireAuthAndCsrf() {
  if (!requireAuth()) return false;
  if (!csrfOk()) {
    server.send(403, "text/plain", "CSRF check failed");
    return false;
  }
  return true;
}

// Chunked HTML streaming (see handleRoot). The settings page is ~62 KB; built
// as one String it forced a ~62 KB heap allocation plus reallocations. Instead
// we open a chunked response and flush the buffer whenever it passes a small
// threshold, so peak transient memory stays a few KB regardless of page size.
static const size_t HTML_FLUSH_THRESHOLD = 2048;

static void htmlBegin() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
}

// Send and clear the buffer once it is worth a chunk. Cheap to call often -
// a no-op while the buffer is still small.
static void htmlFlush(String& page) {
  if (page.length() >= HTML_FLUSH_THRESHOLD) {
    server.sendContent(page);
    page = "";
  }
}

// Flush any remainder and write the terminating empty chunk.
static void htmlEnd(String& page) {
  if (page.length()) server.sendContent(page);
  server.sendContent("");
  page = "";
}

static String colorKeyLabel(const String& key) {
  if (key == "theme") return TXTW(L_WEB_THEME);
  if (key == "standard") return "Standard";
  const PaletteEntry* e = findPaletteEntry(key);
  return e ? String(paletteLabel(*e)) : key;
}

// Shared by every color picker on the Design tab (accent, border, text,
// menu/button/widget background, and the page background theme) so they
// all draw from the exact same palette. allowTheme adds a "follow the
// current design theme" swatch as the first option (used by
// menu/button/widget background, which can also just inherit a panel tone).
static void appendColorPaletteRow(String& page, const String& label, const String& fieldName, const String& selectedKey, const String& valueId, bool allowTheme) {
  page += "<div class='color-row'><div class='color-meta'><label class='label'>" + label + "</label><span class='color-value' id='" + valueId + "'>";
  page += colorKeyLabel(selectedKey);
  page += "</span></div><div class='swatch-row'>";
  if (allowTheme) {
    page += "<label class='swatch" + String(selectedKey == "theme" ? " active" : "") + "' style='background:linear-gradient(135deg,#0f172a,#334155);'><input type='radio' name='" + fieldName + "' value='theme' data-label='" + String(TXTW(L_WEB_THEME)) + "'" + String(selectedKey == "theme" ? " checked" : "") + "></label>";
  }
  for (int i = 0; i < COLOR_PALETTE_COUNT; i++) {
    String key = String(COLOR_PALETTE[i].key);
    page += "<label class='swatch" + String(selectedKey == key ? " active" : "") + "' style='background:" + accentPreviewCss(key) + ";'><input type='radio' name='" + fieldName + "' value='" + key + "' data-label='" + String(paletteLabel(COLOR_PALETTE[i])) + "'" + String(selectedKey == key ? " checked" : "") + "></label>";
  }
  page += "</div></div>";
}



void swapMqttDataPoints(int a, int b) {
  if (a < 0 || b < 0 || a >= mqttValueCount || b >= mqttValueCount) return;
  String label = mqttLabels[a];
  String topic = mqttTopics[a];
  String unit = mqttUnits[a];
  int decimals = mqttDecimals[a];
  bool enabled = mqttValueEnabled[a];
  String value = mqttValues[a];

  mqttLabels[a] = mqttLabels[b];
  mqttTopics[a] = mqttTopics[b];
  mqttUnits[a] = mqttUnits[b];
  mqttDecimals[a] = mqttDecimals[b];
  mqttValueEnabled[a] = mqttValueEnabled[b];
  mqttValues[a] = mqttValues[b];

  mqttLabels[b] = label;
  mqttTopics[b] = topic;
  mqttUnits[b] = unit;
  mqttDecimals[b] = decimals;
  mqttValueEnabled[b] = enabled;
  mqttValues[b] = value;

  lastMqttLine[a] = "";
  lastMqttLine[b] = "";
}

static String tabPanelClass(const String& activeTab, const char* tab) {
  return String("panel") + String(activeTab == tab ? " tab-visible" : "");
}

static String tabButtonClass(const String& activeTab, const char* tab) {
  return String("tab-button") + String(activeTab == tab ? " active" : "");
}

static void appendDataPointsPanel(String& page, const String& sourceFilter, const String& query, const String& activeTab) {
  String normalizedSourceFilter = sourceFilter;
  normalizedSourceFilter.trim();
  if (normalizedSourceFilter.length() == 0 || normalizedSourceFilter == "Alle") normalizedSourceFilter = "all";
  String queryLower = query;
  queryLower.toLowerCase();

  int total = dataPointManager.count();
  int visible = 0;
  int sourceWeather = 0;
  int sourceMqtt = 0;
  int sourceSystem = 0;
  int sourceInternal = 0;
  int sourceOther = 0;

  for (int i = 0; i < total; i++) {
    const DataPoint* point = dataPointManager.at(i);
    if (!point) continue;
    switch (point->source) {
      case DATA_SOURCE_WEATHER: sourceWeather++; break;
      case DATA_SOURCE_MQTT: sourceMqtt++; break;
      case DATA_SOURCE_SYSTEM: sourceSystem++; break;
      case DATA_SOURCE_INTERNAL: sourceInternal++; break;
      default: sourceOther++; break;
    }
  }

  page += "<div class='" + tabPanelClass(activeTab, "datapoints") + "' data-panel='datapoints' data-tab='datapoints'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_DATAPOINTS)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_DP_DESC)) + "</p>";
  page += "<form method='get' action='/' class='bar datapoint-filter'>";
  page += "<input type='hidden' name='tab' value='datapoints'>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_SEARCH)) + "</label><input name='q' value='" + htmlEscape(query) + "' placeholder='" + String(TXTW(L_WEB_DP_SEARCH_PH)) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_SOURCE)) + "</label><select name='src'>";
  const char* sourceKeys[] = {"all", "Weather", "MQTT", "System", "Internal"};
  const char* sourceLabels[] = {TXTW(L_WEB_ALL), TXTW(L_TAB_WEATHER), "MQTT", "System", "Internal"};
  for (int i = 0; i < 5; i++) {
    page += "<option value='" + String(sourceKeys[i]) + "'" + String(normalizedSourceFilter == sourceKeys[i] ? " selected" : "") + ">" + String(sourceLabels[i]) + "</option>";
  }
  page += "</select></div>";
  page += "<button type='submit'>" + String(TXTW(L_WEB_APPLY)) + "</button>";
  page += "<a class='btn secondary' href='/?tab=datapoints'>" + String(TXTW(L_WEB_RESET)) + "</a>";
  page += "</form>";

  page += "<div class='summary'>";
  page += "<div class='card'><span class='muted'>" + String(TXTW(L_WEB_TOTAL)) + "</span><b>" + String(total) + "</b></div>";
  page += "<div class='card'><span class='muted'>" + String(TXTW(L_TAB_WEATHER)) + "</span><b>" + String(sourceWeather) + "</b></div>";
  page += "<div class='card'><span class='muted'>MQTT</span><b>" + String(sourceMqtt) + "</b></div>";
  page += "<div class='card'><span class='muted'>System</span><b>" + String(sourceSystem) + "</b></div>";
  page += "<div class='card'><span class='muted'>Internal</span><b>" + String(sourceInternal + sourceOther) + "</b></div>";
  page += "</div>";

  page += "<div class='tablewrap'><table class='datapoint-table'><thead><tr><th>ID</th><th>Name</th><th>" + String(TXTW(L_WEB_VALUE)) + "</th><th>" + String(TXTW(L_WEB_UNIT)) + "</th><th>" + String(TXTW(L_WEB_SOURCE)) + "</th><th>" + String(TXTW(L_WEB_AGE)) + "</th><th>" + String(TXTW(L_WEB_STATUS)) + "</th></tr></thead><tbody>";
  for (int i = 0; i < total; i++) {
    const DataPoint* point = dataPointManager.at(i);
    if (!point) continue;
    String sourceText = dataPointSourceText(point->source);
    if (normalizedSourceFilter != "all" && normalizedSourceFilter != sourceText) continue;
    if (queryLower.length() > 0) {
      String haystack = point->id + " " + point->label + " " + point->value + " " + point->unit + " " + sourceText;
      haystack.toLowerCase();
      if (haystack.indexOf(queryLower) < 0) continue;
    }
    visible++;
    page += "<tr>";
    page += "<td class='muted'>" + htmlEscape(point->id) + "</td>";
    page += "<td>" + htmlEscape(point->label) + "</td>";
    page += "<td>" + htmlEscape(point->value) + "</td>";
    page += "<td class='muted'>" + htmlEscape(point->unit) + "</td>";
    page += "<td class='muted'>" + htmlEscape(sourceText) + "</td>";
    page += "<td class='muted'>" + htmlEscape(dataPointAgeText(*point)) + "</td>";
    page += "<td class='" + String(point->valid ? "ok" : "bad") + "'>" + String(point->valid ? "OK" : "invalid") + "</td>";
    page += "</tr>";
    htmlFlush(page);
  }
  if (visible == 0) {
    page += "<tr><td colspan='7' class='muted'>" + String(TXTW(L_WEB_DP_NONE)) + "</td></tr>";
  }
  page += "</tbody></table></div>";
  page += "<p class='footer-note'>" + String(TXTW(L_WEB_DP_VISIBLE_PRE)) + String(visible) + String(TXTW(L_WEB_DP_VISIBLE_OF)) + String(total) + String(TXTW(L_WEB_DP_VISIBLE_SUF)) + "</p>";
  page += "</div></div>";
}

// Replaces the browser-native (locale-dependent) file input button with a
// styled label + translated text and a filename span updated via JS, so the
// picker follows the selected UI language instead of the browser language.
static void appendFilePicker(String& page, const char* id, const char* accept) {
  page += "<label class='secondary-button' style='background:#111827;color:#cbd5e1;' for='" + String(id) + "'>" + String(TXTW(L_WEB_CHOOSE_FILE)) + "</label>";
  page += "<input type='file' id='" + String(id) + "' accept='" + String(accept) + "' style='display:none' onchange='fileName(this)'>";
  page += "<span class='muted' id='" + String(id) + "-name'>" + String(TXTW(L_WEB_NO_FILE)) + "</span>";
}

static String deviceDisplayName(const String& name) {
  if (name.length() > 0) return name;
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  if (mac.length() >= 4) return "ESP32-" + mac.substring(mac.length() - 4);
  return "InfoTerm";
}

void handleRoot() {
  syncDataPointsFromRuntime();

  String sourceFilter = server.hasArg("src") ? server.arg("src") : "all";
  String query = server.hasArg("q") ? server.arg("q") : "";
  query.trim();
  String activeTab = server.hasArg("tab") ? server.arg("tab") : "home";
  bool activeTabKnown = activeTab == "home" || activeTab == "mqtt" || activeTab == "settings" || activeTab == "design" || activeTab == "datapoints";
  if (!activeTabKnown) {
    for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
      if (activeTab == String("tab") + String(t + 1)) {
        activeTabKnown = true;
        break;
      }
    }
  }
  if (!activeTabKnown) activeTab = "home";
  String accent = cfgGetString("accent", "accent", "cyan");
  String border = cfgGetString("border", "border", "cyan");
  if (border == "theme" || border.length() == 0) border = "cyan";
  String bg     = cfgGetString("bg", "bg", "petrol");
  String txt    = cfgGetString("text", "text", "standard");
  String menuBg = cfgGetString("menuBg", "menuBg", "theme");
  String buttonBg = cfgGetString("buttonBg", "buttonBg", "theme");
  String widgetBg = cfgGetString("widgetBg", "widgetBg", "theme");
  bool webWidgetBordersEnabled = widgetBordersEnabled;
  bool webNightModeEnabled = nightModeEnabled;
  bool webDisplayRotated180 = displayRotated180;
  String units  = unitKey;
  String region = regionFormatKey;
  String tz     = timezoneKey;
  String deviceName = ::deviceName;
  bool flashMode = flashModeEnabled;
  bool webMqttOn = mqttWeatherEnabled;
  bool webMqttFallback = mqttFallbackOpenMeteo;
  String webMqttHost = mqttHost;
  uint16_t webMqttPort = mqttPort;
  String webMqttUser = mqttUser;
  String webMqttTopic = mqttWeatherflowTopic;
  // Use the current runtime widget state as the single source of truth for the WebGUI.
  // Preferences are only persistent storage; after add/delete/update operations they can
  // briefly lag behind the display state and would show stale selections in the form.
  String homeSlotKeys[HOME_SLOT_COUNT];
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    homeSlotKeys[i] = widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]);
  }
  String customTabSlotKeys[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT];
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      customTabSlotKeys[t][i] = widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]);
    }
  }

  String page;
  page.reserve(3000);
  htmlBegin();

  page += "<!doctype html><html><head>";
  page += "<meta charset='utf-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>InfoTerm</title>";
  page += "<style>body{margin:0;background:#0b1018;color:#edf2f7;font-family:system-ui,sans-serif;font-size:14px;line-height:1.35}.wrap{max-width:980px;margin:0 auto;padding:18px 14px}.hero,.panel{background:#171b22;border:1px solid #2d3748;border-radius:16px;padding:14px;margin-bottom:14px}.hero h1{font-size:28px;line-height:1.1;margin:0 0 10px}.tabs{display:flex;gap:8px;margin-bottom:14px;overflow-x:auto}.tab-button,button,.secondary-button{display:inline-flex;align-items:center;justify-content:center;min-height:36px;background:#111827;border:1px solid #334155;color:#cbd5e1;padding:0 14px;border-radius:12px;font-weight:700;font-size:14px;line-height:1;text-decoration:none}.tab-button.active,button[type=submit]{background:#38bdf8;color:#001018}.panel-toggle{all:unset;display:flex;align-items:center;justify-content:space-between;width:100%;min-height:28px;cursor:pointer;color:#edf2f7}.panel-toggle h2{margin:0;font-size:17px;line-height:1.2}.panel-body{margin-top:12px}.panel.collapsed .panel-body{display:none}.panel[data-tab]{display:none}.panel[data-tab].tab-visible{display:block}.grid,.grid-3{display:grid;grid-template-columns:1fr 1fr;gap:12px}input,select,textarea{width:100%;box-sizing:border-box;border-radius:10px;border:1px solid #334155;background:#0b1220;color:#edf2f7;padding:9px;font-size:14px}.label{display:block;margin:0 0 5px}.color-value{color:#a0aec0;font-size:12px}.color-row{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;padding:8px 0;border-bottom:1px solid rgba(148,163,184,.14)}.color-meta{display:flex;flex-direction:column;flex-shrink:0}.swatch-row{display:flex;flex-wrap:wrap;gap:6px;justify-content:flex-end}.swatch{display:inline-block;width:18px;height:18px;border-radius:50%;border:1px solid #fff4}.swatch input{display:none}.swatch.active{outline:2px solid #67e8f9}.mqtt-config-table,.datapoint-table{width:100%;border-collapse:collapse;font-size:12px}.mqtt-config-table td,.mqtt-config-table th,.datapoint-table td,.datapoint-table th{border-bottom:1px solid #334155;padding:5px 6px}.mqtt-row-actions{display:flex;gap:5px}.mqtt-action-btn,.mqtt-trash{display:inline-flex;align-items:center;justify-content:center;width:28px;height:28px;min-height:28px;padding:0;border:1px solid #334155;border-radius:8px;text-decoration:none;background:#111827;color:#cbd5e1;cursor:pointer;font-weight:400}.datapoint-link{display:inline-flex;align-items:center;min-height:32px;color:#7dd3fc;text-decoration:underline}.status-hero{display:grid;grid-template-columns:170px minmax(0,1fr);gap:24px;align-items:start;border-radius:14px;padding:15px 18px;background:#151922;border-color:#2b3545;min-width:0}.brand-block h1{margin:0;font-size:28px;line-height:1.05}.status-columns{display:grid;grid-template-columns:max-content max-content max-content max-content;column-gap:32px;align-items:start;min-width:0}.status-group{display:grid;grid-template-columns:max-content max-content;column-gap:10px;row-gap:4px;align-items:baseline;min-width:0}.status-group span{color:#fff;font-size:13px}.status-group b{font-size:13px;color:#7dd3fc;font-weight:800;white-space:nowrap}.switch-row{display:flex;align-items:center;justify-content:flex-start;gap:16px;padding:10px 0;border-bottom:1px solid rgba(148,163,184,.14)}.switch-row b{display:block}.switch-row small{display:block;color:#8ea3ba;margin-top:3px}.switch-row input{width:auto}.settings-block{margin-top:16px}.muted,.settings-desc,.footer-note{color:#8ea3ba;font-size:13px;margin-top:4px}.settings-title{font-weight:700}.bar{display:flex;gap:10px;flex-wrap:wrap;align-items:end;margin:12px 0}.summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:10px;margin:12px 0}.card,.tablewrap{background:#171b22;border:1px solid #2d3748;border-radius:14px;padding:12px}.tablewrap{overflow-x:auto}.card b{display:block;font-size:18px;margin-top:4px}.btn.secondary{display:inline-flex;align-items:center;justify-content:center;min-height:36px;background:#111827;color:#cbd5e1;border:1px solid #334155;padding:0 14px;border-radius:10px;text-decoration:none;font-weight:700}.ok{color:#86efac}.bad{color:#fca5a5}@media(max-width:1000px){.status-columns{grid-template-columns:1fr 1fr}.status-group{grid-template-columns:96px 1fr}.status-group b{text-align:left}}@media(max-width:820px){.status-hero{grid-template-columns:1fr;gap:12px}.status-columns{grid-template-columns:1fr 1fr;gap:10px 18px}.grid,.grid-3{grid-template-columns:1fr}}@media(max-width:520px){.status-columns{grid-template-columns:1fr}.status-group{grid-template-columns:86px 1fr}}</style>";
  htmlFlush(page);  // the CSS block above is the single biggest append (~3.5 KB)
  page += "</head><body><div class='wrap'>";
  page += "<div class='hero status-hero'>";
  page += "<div class='brand-block'><h1>InfoTerm</h1></div>";
  page += "<div class='status-columns'>";
  page += "<div class='status-group'>";
  page += "<span>" + String(TXTW(L_WEB_DEVICE_NAME)) + "</span><b>" + htmlEscape(deviceDisplayName(deviceName)) + "</b>";
  page += "<span>" + String(TXTW(L_STATUS_VERSION)) + "</span><b>" + String(INFOTERM_APP_VERSION) + "</b>";
  page += "<span>" + String(TXTW(L_STATUS_UPTIME)) + "</span><b>" + htmlEscape(uptimeText()) + "</b>";
  page += "</div>";
  page += "<div class='status-group'>";
  page += "<span>" + String(TXTW(L_STATUS_WIFI)) + "</span><b>" + htmlEscape(wifiStatusText()) + "</b>";
  page += "<span>" + String(TXTW(L_STATUS_IP)) + "</span><b>" + htmlEscape(WiFi.localIP().toString()) + "</b>";
  page += "<span>" + String(TXTW(L_STATUS_SIGNAL)) + "</span><b>" + htmlEscape(signalText()) + "</b>";
  page += "</div>";
  page += "<div class='status-group'>";
  page += "<span>MQTT</span><b>" + htmlEscape(mqttStateText()) + "</b>";
  page += "<span>" + String(TXTW(L_STATUS_BROKER)) + "</span><b>" + htmlEscape(mqttHost) + ":" + String(mqttPort) + "</b>";
  page += "<span>Sync</span><b>" + htmlEscape(lastSyncValueText()) + "</b>";
  page += "</div>";
  page += "<div class='status-group'>";
  page += "<span>VPN</span><b>" + htmlEscape(vpnStatusText()) + "</b>";
  page += "<span>Name</span><b>" + htmlEscape(vpnName.length() ? vpnName : String("-")) + "</b>";
  page += "<span>" + String(TXTW(L_WEB_TUNNEL_IP)) + "</span><b>" + htmlEscape(vpnConnected ? vpnLocalAddress : String("-")) + "</b>";
  page += "</div>";
  page += "</div>";
  page += "</div>";

  page += "<form method='POST' action='/save'>";
  page += "<input type='hidden' name='csrf' value='" + webCsrfToken + "'>";
  page += "<div class='tabs' role='tablist'>";
  page += "<a class='" + tabButtonClass(activeTab, "home") + "' href='/?tab=home' data-target-tab='home'>" + String(TXTW(L_TAB_START)) + "</a>";
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabLabel = customTabNames[t];
    tabLabel.trim();
    if (tabLabel.length() == 0) tabLabel = "Tab" + String(t + 1);
    String tabId = String("tab") + String(t + 1);
    page += "<a class='" + tabButtonClass(activeTab, tabId.c_str()) + "' href='/?tab=" + tabId + "' data-target-tab='" + tabId + "'>" + htmlEscape(tabLabel) + "</a>";
  }
  page += "<a class='" + tabButtonClass(activeTab, "mqtt") + "' href='/?tab=mqtt' data-target-tab='mqtt'>MQTT</a>";
  page += "<a class='" + tabButtonClass(activeTab, "datapoints") + "' href='/?tab=datapoints' data-target-tab='datapoints'>" + String(TXTW(L_WEB_DATAPOINTS)) + "</a>";
  page += "<a class='" + tabButtonClass(activeTab, "design") + "' href='/?tab=design' data-target-tab='design'>" + String(TXTW(L_WEB_THEME_TITLE)) + "</a>";
  page += "<a class='" + tabButtonClass(activeTab, "settings") + "' href='/?tab=settings' data-target-tab='settings'>" + String(TXTW(L_WEB_SETTINGS)) + "</a>";
  page += "</div>";

  if (activeTab == "datapoints") {
    page += "</form>";
    appendDataPointsPanel(page, sourceFilter, query, activeTab);
    page += "</div></body></html>";
    htmlEnd(page);
    return;
  }

  appendHomeWidgetOptionTemplate(page);
  appendDataPointWidgetOptionTemplate(page);
  page += "<script>function hydrateWidgetSelect(id){var sel=document.getElementById(id);if(!sel||sel.getAttribute('data-dp-ready'))return;var widgetTpl=document.getElementById('widget-options');if(widgetTpl)sel.insertAdjacentHTML('beforeend',widgetTpl.innerHTML);var dpTpl=document.getElementById('dp-widget-options');if(dpTpl)sel.insertAdjacentHTML('beforeend',dpTpl.innerHTML);var selected=sel.getAttribute('data-selected');if(selected){sel.value=selected;}sel.setAttribute('data-dp-ready','1');}</script>";
  page += "<div class='stack tab-panels'>";
  htmlFlush(page);

  page += "<div class='" + tabPanelClass(activeTab, "mqtt") + "' data-panel='mqtt' data-tab='mqtt'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_MQTT_WEATHER_TITLE)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<div class='settings-block'>";
  page += "<div class='grid'>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_BROKER)) + "</label><input name='mqttHost' value='" + htmlEscape(webMqttHost) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_PORT)) + "</label><input name='mqttPort' type='number' min='1' max='65535' value='" + String(webMqttPort) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_USER)) + "</label><input name='mqttUser' value='" + htmlEscape(webMqttUser) + "'></div>";
  // The stored password is never rendered into the page (/ is reachable
  // without auth); an empty field means "keep the stored password".
  page += "<div><label class='label'>" + String(TXTW(L_WEB_PASSWORD)) + "</label><input name='mqttPass' type='password' value='' autocomplete='new-password' placeholder='" + String(mqttPass.length() > 0 ? TXTW(L_WEB_PASSWORD_KEEP_HINT) : "") + "'>";
  if (mqttPass.length() > 0) {
    page += "<label class='muted' style='display:flex;align-items:center;gap:6px;margin-top:6px;'><input type='checkbox' name='mqttPassClear' value='1' style='width:auto;'>" + String(TXTW(L_WEB_PASSWORD_CLEAR)) + "</label>";
  }
  page += "</div>";
  page += "</div>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:12px;'>";
  page += "<button type='button' class='secondary-button' onclick='submitMqttTest()'>" + String(TXTW(L_WEB_TEST_CONNECTION)) + "</button>";
  page += "<span id='mqttTestResult' class='muted'></span>";
  page += "</div>";
  page += "<label class='label' style='margin-top:12px;'>" + String(TXTW(L_WEB_OBSERVATION_TOPIC)) + "</label>";
  page += "<input name='mqttTopic' value='" + htmlEscape(webMqttTopic) + "'>";
  page += "<label class='switch-row' style='margin-top:14px;border-bottom:none;'><input type='checkbox' name='mqttOn' value='1' ";
  page += webMqttOn ? "checked" : "";
  page += "><small>" + String(TXTW(L_WEB_USE_MQTT_WEATHER)) + "</small></label>";
  page += "<label class='switch-row' style='border-bottom:none;'><input type='checkbox' name='mqttFallback' value='1' ";
  page += webMqttFallback ? "checked" : "";
  page += "><small>" + String(TXTW(L_WEB_USE_OPENMETEO_FALLBACK)) + "</small></label>";
  page += "</div>";
  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "settings") + "' data-panel='settings' data-tab='settings'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_SETTINGS)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_SETTINGS_DESC)) + "</p>";
  page += "<div class='settings-block'>";
  page += "<span class='settings-title'>" + String(TXTW(L_WEB_GENERAL)) + "</span>";
  page += "<div class='grid'>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_DEVICE_NAME)) + "</label><input name='deviceName' maxlength='24' value='" + htmlEscape(deviceName) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_WIFI_NETWORK)) + "</label><select name='wifiNet'>";
  for (int i = 0; i < WIFI_NETWORK_COUNT; i++) {
    String suffix;
    bool isConnectedNet = WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(WIFI_NETWORKS[i].ssid);
    if (isConnectedNet) {
      suffix = TXTW(L_WEB_WIFI_CONNECTED);
    } else if (!wifiScanEverCompleted) {
      suffix = "";
    } else if (wifiNetworkReachable[i]) {
      suffix = TXTW(L_WEB_WIFI_IN_RANGE);
    } else {
      suffix = TXTW(L_WEB_WIFI_OUT_RANGE);
    }
    page += "<option value='" + String(i) + "'" + String(i == wifiActiveIndex ? " selected" : "") +
            ">" + htmlEscape(String(WIFI_NETWORKS[i].ssid) + suffix) + "</option>";
  }
  page += "</select><div class='muted' style='margin-top:8px;'>" + String(TXTW(L_WEB_WIFI_HINT)) + "</div></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_LANGUAGE)) + "</label><select name='lang'>";
  page += "<option value='de'" + String(uiLanguage==LANG_DE ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_DE)) + "</option>";
  page += "<option value='en'" + String(uiLanguage==LANG_EN ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_EN)) + "</option>";
  page += "<option value='fr'" + String(uiLanguage==LANG_FR ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_FR)) + "</option>";
  page += "<option value='es'" + String(uiLanguage==LANG_ES ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_ES)) + "</option>";
  page += "<option value='it'" + String(uiLanguage==LANG_IT ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_IT)) + "</option>";
  page += "<option value='ru'" + String(uiLanguage==LANG_RU ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_RU)) + "</option>";
  page += "<option value='hi'" + String(uiLanguage==LANG_HI ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_HI)) + "</option>";
  page += "<option value='zh'" + String(uiLanguage==LANG_ZH ? " selected" : "") + ">" + String(TXTW(L_WEB_LANGUAGE_ZH)) + "</option>";
  page += "</select></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_AUTO_SLEEP)) + "</label><select name='sleepMin'>";
  page += "<option value='0'"  + String(sleepIntervalMin==0?" selected":"")  + ">" + String(TXTW(L_WEB_NEVER)) + "</option>";
  page += "<option value='1'"  + String(sleepIntervalMin==1?" selected":"")  + ">" + String(TXTW(L_WEB_1_MINUTE)) + "</option>";
  page += "<option value='5'"  + String(sleepIntervalMin==5?" selected":"")  + ">" + String(TXTW(L_WEB_5_MINUTES)) + "</option>";
  page += "<option value='10'" + String(sleepIntervalMin==10?" selected":"") + ">" + String(TXTW(L_WEB_10_MINUTES)) + "</option>";
  page += "<option value='30'" + String(sleepIntervalMin==30?" selected":"") + ">" + String(TXTW(L_WEB_30_MINUTES)) + "</option>";
  page += "<option value='60'" + String(sleepIntervalMin==60?" selected":"") + ">" + String(TXTW(L_WEB_1_HOUR)) + "</option>";
  page += "</select><div class='muted' style='margin-top:8px;'>" + String(TXTW(L_WEB_SLEEP_HINT)) + "</div></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_TAB_CYCLE)) + "</label>";
  page += "<label class='switch-row' style='padding-top:0;border-bottom:none;'><input type='checkbox' name='tabCycle' value='1'" + String(tabCycleEnabled ? " checked" : "") + "><small>" + String(TXTW(L_WEB_TAB_CYCLE_HINT)) + "</small></label>";
  page += "<select name='tabCycleSec'>";
  page += "<option value='30'"  + String(tabCycleIntervalSec==30?" selected":"")  + ">" + String(TXTW(L_WEB_EVERY_30S)) + "</option>";
  page += "<option value='60'"  + String(tabCycleIntervalSec==60?" selected":"")  + ">" + String(TXTW(L_WEB_EVERY_60S)) + "</option>";
  page += "<option value='180'" + String(tabCycleIntervalSec==180?" selected":"") + ">" + String(TXTW(L_WEB_EVERY_3M)) + "</option>";
  page += "<option value='360'" + String(tabCycleIntervalSec==360?" selected":"") + ">" + String(TXTW(L_WEB_EVERY_6M)) + "</option>";
  page += "</select></div>";
  page += "<div><label class='label'>Log-Level</label><select name='logLevel'>";
  page += "<option value='0'" + String(logLevelValue==0?" selected":"") + ">ERROR</option>";
  page += "<option value='1'" + String(logLevelValue==1?" selected":"") + ">WARNING</option>";
  page += "<option value='2'" + String(logLevelValue==2?" selected":"") + ">INFO</option>";
  page += "<option value='3'" + String(logLevelValue==3?" selected":"") + ">DEBUG</option>";
  page += "</select></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_UNITS)) + "</label><select name='units'>";
  page += "<option value='metric'"   + String(units=="metric"?" selected":"")   + ">Celsius / mm</option>";
  page += "<option value='imperial'" + String(units=="imperial"?" selected":"") + ">" + String(TXTW(L_WEB_UNIT_IMPERIAL)) + "</option>";
  page += "</select></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_DATE_FORMAT)) + "</label><select name='region'>";
  page += "<option value='europe'" + String(region=="europe"?" selected":"") + ">" + String(TXTW(L_WEB_DATE_EUROPE)) + "</option>";
  page += "<option value='us'" + String(region=="us"?" selected":"") + ">" + String(TXTW(L_WEB_DATE_US)) + "</option>";
  page += "</select></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_TIMEZONE)) + "</label><select name='tz'>";
  appendTimezoneOptions(page, tz);
  page += "</select></div>";
  page += "</div>";
  page += "</div>";
  page += "<div class='settings-block'><span class='settings-title'>" + String(TXTW(L_WEB_LOCATION)) + "</span><div class='settings-desc'>" + String(TXTW(L_WEB_LOCATION_DESC)) + "</div><div class='grid-3'>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_LOCATION_NAME)) + "</label><input name='locname' value='" + htmlEscape(locationName) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_LATITUDE)) + "</label><input name='lat' value='" + String(LAT, 6) + "'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_LONGITUDE)) + "</label><input name='lng' value='" + String(LNG, 6) + "'></div>";
  page += "</div><div class='footer-note'>" + String(TXTW(L_WEB_LOCATION_EXAMPLE)) + "</div></div>";

  page += "<div class='settings-block'><span class='settings-title'>" + String(TXTW(L_WEB_NETWORK_IP)) + "</span>";
  page += "<div class='settings-desc'>" + String(TXTW(L_WEB_IP_DESC)) + "</div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_IP_MODE)) + "</label><select name='ipMode'>";
  page += "<option value='dhcp'" + String(ipMode == "static" ? "" : " selected") + ">" + String(TXTW(L_WEB_IP_DHCP)) + "</option>";
  page += "<option value='static'" + String(ipMode == "static" ? " selected" : "") + ">" + String(TXTW(L_WEB_IP_STATIC)) + "</option>";
  page += "</select></div>";
  page += "<div class='grid'>";
  page += "<div><label class='label'>" + String(TXTW(L_STATUS_IP)) + "</label><input name='ipStatic' value='" + htmlEscape(staticIp) + "' placeholder='192.168.1.50'></div>";
  page += "<div><label class='label'>Gateway</label><input name='ipGw' value='" + htmlEscape(staticGateway) + "' placeholder='192.168.1.1'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_SUBNET)) + "</label><input name='ipMask' value='" + htmlEscape(staticSubnet) + "' placeholder='255.255.255.0'></div>";
  page += "<div><label class='label'>" + String(TXTW(L_WEB_DNS)) + "</label><input name='ipDns' value='" + htmlEscape(staticDns) + "' placeholder='192.168.1.1'></div>";
  page += "</div></div>";

  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "settings") + "' data-panel='vpn' data-tab='settings'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>VPN</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_VPN_DESC)) + "</p>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-bottom:14px;'>";
  appendFilePicker(page, "vpnFileInput", ".conf");
  page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='submitVpnUpload()'>" + String(TXTW(L_WEB_UPLOAD)) + "</button>";
  page += "</div>";
  if (vpnPrivateKey.length() > 0) {
    String peerKeyShort = vpnPeerPublicKey.length() > 12 ? vpnPeerPublicKey.substring(0, 12) + "..." : vpnPeerPublicKey;
    page += "<div class='grid'>";
    page += "<div><label class='label'>Name</label><input name='vpnName' maxlength='24' value='" + htmlEscape(vpnName) + "'></div>";
    page += "<div><label class='label'>" + String(TXTW(L_WEB_ENDPOINT)) + "</label><input value='" + htmlEscape(vpnEndpointHost) + ":" + String(vpnEndpointPort) + "' disabled></div>";
    page += "<div><label class='label'>Peer-Key</label><input value='" + htmlEscape(peerKeyShort) + "' disabled></div>";
    page += "<div><label class='label'>" + String(TXTW(L_WEB_TUNNEL_ADDR)) + "</label><input value='" + htmlEscape(vpnLocalAddress) + "' disabled></div>";
    page += "</div>";
    page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:14px;'>";
    page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='submitVpnConnect()'>" + String(TXTW(L_WEB_CONNECT)) + "</button>";
    page += "<button type='button' class='secondary-button' onclick='submitVpnDisconnect()'>" + String(TXTW(L_WEB_DISCONNECT)) + "</button>";
    page += "<span class='muted'>" + String(TXTW(L_WEB_STATUS)) + ": " + htmlEscape(vpnStatusMessage) + "</span>";
    page += "</div>";
  } else {
    page += "<p class='footer-note'>" + String(TXTW(L_WEB_VPN_NONE)) + "</p>";
  }
  page += "</div></div>";

  if (activeTab == "settings") {
    page += "<div class='savebar' style='margin-bottom:22px;'>";
    page += "<button type='submit'>" + String(TXTW(L_WEB_SAVE)) + "</button>";
    page += "</div>";
  }

  page += "<div class='" + tabPanelClass(activeTab, "settings") + "' data-panel='backup' data-tab='settings'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>Backup</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_BACKUP_DESC)) + "</p>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;'>";
  page += "<a class='secondary-button' style='background:#38bdf8;color:#001018;' href='/backup/export'>" + String(TXTW(L_WEB_BACKUP_DOWNLOAD)) + "</a>";
  appendFilePicker(page, "backupFileInput", ".json");
  page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='submitBackupImport()'>" + String(TXTW(L_WEB_RESTORE)) + "</button>";
  page += "</div>";
  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "settings") + "' data-panel='firmware' data-tab='settings'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_FW_TITLE)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_FW_CURRENT_VERSION)) + String(INFOTERM_APP_VERSION) + String(TXTW(L_WEB_FW_DESC)) + "</p>";
  page += "<p class='footer-note'>" + String(TXTW(L_WEB_FW_WARN)) + "</p>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;'>";
  appendFilePicker(page, "otaFileInput", ".bin");
  page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='submitOtaUpload()'>" + String(TXTW(L_WEB_FW_UPLOAD)) + "</button>";
  page += "<span id='otaStatus' class='muted'></span>";
  page += "</div>";
  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "settings") + "' data-panel='log' data-tab='settings'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>Log</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_LOG_DESC_PRE)) + String(Logger::HISTORY_SIZE) + String(TXTW(L_WEB_LOG_DESC_POST)) + "</p>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-bottom:10px;'>";
  page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='loadLogView()'>" + String(TXTW(L_WEB_LOG_LOAD)) + "</button>";
  page += "</div>";
  page += "<pre id='logOutput' style='background:#0b1220;border:1px solid #334155;border-radius:10px;padding:10px;font-size:12px;line-height:1.4;max-height:320px;overflow:auto;white-space:pre-wrap;margin:0;'></pre>";
  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "design") + "' data-panel='theme' data-tab='design'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_THEME_TITLE)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_THEME_DESC)) + "</p>";
  page += "<div class='grid'>";

  page += "<div style='grid-column:1 / -1;' class='color-stack'>";
  page += "<b>" + String(TXTW(L_WEB_NIGHT_MODE)) + "</b>";
  page += "<label class='switch-row'><input type='checkbox' name='nightMode' value='1'" + String(webNightModeEnabled ? " checked" : "") + "><small>" + String(TXTW(L_WEB_NIGHT_MODE_HINT)) + "</small></label>";
  page += "<b>" + String(TXTW(L_WEB_WIDGET_BORDER)) + "</b>";
  page += "<label class='switch-row'><input type='checkbox' name='widgetBorder' value='1'" + String(webWidgetBordersEnabled ? " checked" : "") + "><small>" + String(TXTW(L_WEB_WIDGET_BORDER_HINT)) + "</small></label>";
  page += "<b>" + String(TXTW(L_WEB_ROTATE)) + "</b>";
  page += "<label class='switch-row'><input type='checkbox' name='rotate180' value='1'" + String(webDisplayRotated180 ? " checked" : "") + "><small>" + String(TXTW(L_WEB_ROTATE_HINT)) + "</small></label>";

  appendColorPaletteRow(page, String(TXTW(L_WEB_ACCENT)), "accent", accent, "accent-value", false);
  appendColorPaletteRow(page, String(TXTW(L_WEB_BORDER_COLOR)), "border", border, "border-value", false);
  appendColorPaletteRow(page, String(TXTW(L_WEB_TEXT_COLOR)), "text", txt, "text-value", false);
  appendColorPaletteRow(page, String(TXTW(L_WEB_MENU_BG)), "menuBg", menuBg, "menuBg-value", true);
  appendColorPaletteRow(page, String(TXTW(L_WEB_BUTTON_BG)), "buttonBg", buttonBg, "buttonBg-value", true);
  appendColorPaletteRow(page, String(TXTW(L_WEB_WIDGET_BG)), "widgetBg", widgetBg, "widgetBg-value", true);
  appendColorPaletteRow(page, String(TXTW(L_WEB_THEME)), "bg", bg, "bg-value", false);

  page += "</div>";

  page += "</div></div></div>";
  htmlFlush(page);

  page += "<div class='" + tabPanelClass(activeTab, "mqtt") + "' data-panel='mqttvalues' data-tab='mqtt'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_MQTT_VALUES_TITLE)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<div class='settings-block'><div class='settings-desc'>" + String(TXTW(L_WEB_MQTT_CFG_DESC)) + "</div>";
  page += "<input type='hidden' name='mqttValueCount' value='" + String(mqttValueCount) + "'>";
  page += "<div class='datapoint-scroll'><table class='mqtt-config-table'><thead><tr><th></th><th>Name</th><th>Topic</th><th>Unit</th><th>" + String(TXTW(L_WEB_DECIMALS)) + "</th><th>" + String(TXTW(L_WEB_ACTIVE)) + "</th></tr></thead><tbody>";
  for (int i = 0; i < mqttValueCount; i++) {
    page += "<tr>";
    page += "<td><div class='mqtt-row-actions'>";
    if (i > 0) page += "<button type='button' class='mqtt-action-btn' onclick='mqttRowAction(\"/mqtt/move\"," + String(i) + ",\"up\")'>&#9650;</button>";
    else page += "<span class='mqtt-action-btn' style='opacity:.25;pointer-events:none;'>&#9650;</span>";
    if (i < mqttValueCount - 1) page += "<button type='button' class='mqtt-action-btn' onclick='mqttRowAction(\"/mqtt/move\"," + String(i) + ",\"down\")'>&#9660;</button>";
    else page += "<span class='mqtt-action-btn' style='opacity:.25;pointer-events:none;'>&#9660;</span>";
    page += "<button type='button' class='mqtt-trash' title='" + String(TXTW(L_WEB_DP_DELETE_TITLE)) + "' onclick=\"if(confirm('" + String(TXTW(L_WEB_DP_DELETE_CONFIRM)) + "'))mqttRowAction('/mqtt/delete'," + String(i) + ",'')\">&#128465;</button>";
    page += "</div></td>";
    page += "<td><input type='text' name='mqttLabel" + String(i) + "' maxlength='18' value='" + htmlEscape(mqttLabels[i]) + "'></td>";
    page += "<td><input type='text' name='mqttTopic" + String(i) + "' value='" + htmlEscape(mqttTopics[i]) + "' placeholder='topic'></td>";
    page += "<td><input type='text' name='mqttUnit" + String(i) + "' maxlength='8' value='" + htmlEscape(mqttUnits[i]) + "' placeholder='C, %, W'></td>";
    page += "<td><select name='mqttDec" + String(i) + "'>";
    page += "<option value='-1'" + String(mqttDecimals[i] == MQTT_DECIMALS_AUTO ? " selected" : "") + ">Auto</option>";
    for (int d = 0; d <= 4; d++) {
      page += "<option value='" + String(d) + "'" + String(mqttDecimals[i] == d ? " selected" : "") + ">" + String(d) + "</option>";
    }
    page += "</select></td>";
    page += "<td><label class='muted'><input class='mqtt-action' type='checkbox' name='mqttValEn" + String(i) + "' value='1'" + String(mqttValueEnabled[i] ? " checked" : "") + "> " + String(TXTW(L_WEB_ACTIVE)) + "</label></td>";
    page += "</tr>";
    htmlFlush(page);
  }
  page += "</tbody></table></div>";
  page += "<button type='submit' class='secondary-button' name='mqttAdd' value='1'>" + String(TXTW(L_WEB_DP_ADD)) + "</button>";
  page += "<div class='settings-block'><div class='settings-desc'>" + String(TXTW(L_WEB_MQTT_EXPORT_DESC)) + "</div>";
  page += "<div style='display:flex;gap:10px;flex-wrap:wrap;align-items:center;'>";
  page += "<a class='secondary-button' style='background:#38bdf8;color:#001018;' href='/mqtt/export'>" + String(TXTW(L_WEB_DP_EXPORT)) + "</a>";
  appendFilePicker(page, "mqttImportInput", ".json");
  page += "<button type='button' class='secondary-button' style='background:#38bdf8;color:#001018;' onclick='submitMqttImport()'>" + String(TXTW(L_WEB_DP_IMPORT)) + "</button>";
  page += "</div></div>";
  page += "</div>";
  page += "</div></div>";

  page += "<div class='" + tabPanelClass(activeTab, "home") + "' data-panel='widgets' data-tab='home'>";
  page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + String(TXTW(L_WEB_WIDGETS_TITLE)) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
  page += "<div class='panel-body'>";
  page += "<p class='muted'>" + String(TXTW(L_WEB_WIDGETS_DESC)) + "</p>";
  page += "<div class='grid'>";
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    page += "<div><label class='label'>";
    page += homeSlotLabel(i);
    page += "</label><select name='homeSlot";
    page += String(i);
    page += "' id='homeSlotSelect";
    page += String(i);
    page += "' data-widget-select='1' data-selected='";
    page += htmlEscape(homeSlotKeys[i]);
    page += "'>";
    page += "</select><script>hydrateWidgetSelect('homeSlotSelect";
    page += String(i);
    page += "');</script></div>";
    htmlFlush(page);
  }
  page += "</div>";
  page += "</div></div>";

  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabId = String("tab") + String(t + 1);
    page += "<div class='" + tabPanelClass(activeTab, tabId.c_str()) + "' data-panel='" + tabId + "' data-tab='" + tabId + "'>";
    String panelTabLabel = customTabNames[t];
    panelTabLabel.trim();
    if (panelTabLabel.length() == 0) panelTabLabel = "Tab" + String(t + 1);
    page += "<button type='button' class='panel-toggle' aria-expanded='true'><h2>" + htmlEscape(panelTabLabel) + "</h2><span class='panel-chevron'>&#9662;</span></button>";
    page += "<div class='panel-body'>";
    page += "<p class='muted'>" + String(TXTW(L_WEB_TAB_CFG_DESC)) + "</p>";
    page += "<div class='grid'>";
    page += "<div><label class='label'>" + String(TXTW(L_WEB_TAB_NAME)) + "</label><input name='tabName" + String(t) + "' maxlength='10' value='" + htmlEscape(customTabNames[t]) + "'></div>";
    page += "<label class='mqtt-check' style='align-self:end;'><input type='checkbox' name='tabEnabled" + String(t) + "' value='1'" + String(customTabEnabled[t] ? " checked" : "") + " style='width:auto;'>" + String(TXTW(L_WEB_ACTIVE)) + "</label>";
    page += "</div>";
    page += "<div class='settings-block'><span class='settings-title'>Widgets</span><div class='grid'>";
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      page += "<div><label class='label'>";
      page += tabWidgetSlotLabel(i);
      String selectId = String("tab") + String(t) + "SlotSelect" + String(i);
      page += "</label><select name='tab" + String(t) + "Slot" + String(i) + "' id='" + selectId + "' data-widget-select='1' data-selected='";
      page += htmlEscape(customTabSlotKeys[t][i]);
      page += "'>";
      page += "</select><script>hydrateWidgetSelect('" + selectId + "');</script></div>";
    }
    page += "</div></div>";
    page += "</div></div>";
    htmlFlush(page);
  }

  page += "</div>";
  if (activeTab != "datapoints" && activeTab != "settings") {
    page += "<div class='savebar'>";
    page += "<button type='submit'>" + String(TXTW(L_WEB_SAVE)) + "</button>";
    page += "</div>";
  }
  page += "</form>";
  htmlFlush(page);
  page += "<script>";
  page += "var csrfToken='" + webCsrfToken + "';";
  // Localized JS-facing strings (alerts/confirms/status) injected once as an object.
  page += "var T={";
  page += "backupSelect:'" + String(TXTW(L_JS_BACKUP_SELECT)) + "',";
  page += "restoreFailed:'" + String(TXTW(L_JS_RESTORE_FAILED)) + "',";
  page += "fwSelect:'" + String(TXTW(L_JS_FW_SELECT)) + "',";
  page += "fwConfirm:'" + String(TXTW(L_JS_FW_CONFIRM)) + "',";
  page += "uploading:'" + String(TXTW(L_JS_UPLOADING)) + "',";
  page += "updateOk:'" + String(TXTW(L_JS_UPDATE_OK)) + "',";
  page += "updateFailed:'" + String(TXTW(L_JS_UPDATE_FAILED)) + "',";
  page += "fwUpdateFailed:'" + String(TXTW(L_JS_FW_UPDATE_FAILED)) + "',";
  page += "vpnSelect:'" + String(TXTW(L_JS_VPN_SELECT)) + "',";
  page += "vpnUploadFailed:'" + String(TXTW(L_JS_VPN_UPLOAD_FAILED)) + "',";
  page += "vpnConnectConfirm:'" + String(TXTW(L_JS_VPN_CONNECT_CONFIRM)) + "',";
  page += "loading:'" + String(TXTW(L_JS_LOADING)) + "',";
  page += "logFailed:'" + String(TXTW(L_JS_LOG_FAILED)) + "',";
  page += "testing:'" + String(TXTW(L_JS_TESTING)) + "',";
  page += "testFailed:'" + String(TXTW(L_JS_TEST_FAILED)) + "',";
  page += "mqttSelect:'" + String(TXTW(L_JS_MQTT_SELECT)) + "',";
  page += "mqttImportConfirm:'" + String(TXTW(L_JS_MQTT_IMPORT_CONFIRM)) + "',";
  page += "importFailed:'" + String(TXTW(L_JS_IMPORT_FAILED)) + "',";
  page += "noFile:'" + String(TXTW(L_WEB_NO_FILE)) + "'";
  page += "};";
  page += "function fileName(inp){var s=document.getElementById(inp.id+'-name');if(s)s.textContent=(inp.files&&inp.files.length)?inp.files[0].name:T.noFile;}";
  page += "function mqttRowAction(path,idx,dir){var p=new URLSearchParams();p.append('idx',idx);if(dir)p.append('dir',dir);p.append('csrf',csrfToken);fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p}).then(function(){location.href='/?tab=mqtt';}).catch(function(){});}";
  page += "document.body.classList.add('js-tabs');function showTab(tab){document.querySelectorAll('.tab-button').forEach(function(btn){btn.classList.toggle('active',btn.getAttribute('data-target-tab')===tab);});document.querySelectorAll('.panel[data-tab]').forEach(function(panel){panel.classList.toggle('tab-visible',panel.getAttribute('data-tab')===tab);});try{localStorage.setItem('infoterm-active-tab',tab);}catch(e){}}function submitBackupImport(){var input=document.getElementById('backupFileInput');if(!input||!input.files||!input.files.length){alert(T.backupSelect);return;}var fd=new FormData();fd.append('backupFile',input.files[0]);fetch('/backup/import?csrf='+csrfToken,{method:'POST',body:fd}).then(function(r){location.href='/?tab=settings'+(r.ok?'':'&importError=1');}).catch(function(){alert(T.restoreFailed);});}function submitOtaUpload(){var input=document.getElementById('otaFileInput');var status=document.getElementById('otaStatus');if(!input||!input.files||!input.files.length){alert(T.fwSelect);return;}if(!confirm(T.fwConfirm)){return;}var fd=new FormData();fd.append('firmware',input.files[0]);var xhr=new XMLHttpRequest();xhr.open('POST','/ota/upload?csrf='+csrfToken);xhr.upload.onprogress=function(e){if(status&&e.lengthComputable){status.textContent=T.uploading+Math.round(e.loaded*100/e.total)+'%';}};xhr.onload=function(){if(xhr.status===200&&xhr.responseText==='OK'){if(status)status.textContent=T.updateOk;setTimeout(function(){location.href='/?tab=settings';},8000);}else{if(status)status.textContent=T.updateFailed;alert(T.fwUpdateFailed);}};xhr.onerror=function(){if(status)status.textContent=T.updateFailed;alert(T.fwUpdateFailed);};if(status)status.textContent=T.uploading+'0%';xhr.send(fd);}";
  page += "function submitVpnUpload(){var input=document.getElementById('vpnFileInput');if(!input||!input.files||!input.files.length){alert(T.vpnSelect);return;}var fd=new FormData();fd.append('vpnFile',input.files[0]);fetch('/vpn/upload?csrf='+csrfToken,{method:'POST',body:fd}).then(function(r){location.href='/?tab=settings'+(r.ok?'':'&vpnError=1');}).catch(function(){alert(T.vpnUploadFailed);});}";
  page += "function submitVpnConnect(){if(!confirm(T.vpnConnectConfirm)){return;}fetch('/vpn/connect?csrf='+csrfToken,{method:'POST'}).then(function(){setTimeout(function(){location.href='/?tab=settings';},1500);}).catch(function(){});}";
  page += "function loadLogView(){var out=document.getElementById('logOutput');if(!out)return;out.textContent=T.loading;fetch('/log').then(function(r){if(!r.ok)throw 0;return r.text();}).then(function(t){out.textContent=t;}).catch(function(){out.textContent=T.logFailed;});}";
  page += "function submitMqttTest(){var res=document.getElementById('mqttTestResult');function val(n){var el=document.getElementsByName(n)[0];return el?el.value:'';}var p=new URLSearchParams();p.append('mqttHost',val('mqttHost'));p.append('mqttPort',val('mqttPort'));p.append('mqttUser',val('mqttUser'));p.append('mqttPass',val('mqttPass'));p.append('csrf',csrfToken);if(res)res.textContent=T.testing;fetch('/mqtt/test',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p}).then(function(r){return r.text();}).then(function(t){if(res)res.textContent=t;}).catch(function(){if(res)res.textContent=T.testFailed;});}";
  page += "function submitMqttImport(){var input=document.getElementById('mqttImportInput');if(!input||!input.files||!input.files.length){alert(T.mqttSelect);return;}if(!confirm(T.mqttImportConfirm)){return;}var fd=new FormData();fd.append('mqttFile',input.files[0]);fetch('/mqtt/import?csrf='+csrfToken,{method:'POST',body:fd}).then(function(r){location.href='/?tab=mqtt'+(r.ok&&r.url.indexOf('mqttImportError')<0?'':'&mqttImportError=1');}).catch(function(){alert(T.importFailed);});}";
  page += "function submitVpnDisconnect(){fetch('/vpn/disconnect?csrf='+csrfToken,{method:'POST'}).then(function(){location.href='/?tab=settings';}).catch(function(){location.href='/?tab=settings';});}";
  page += "var initialTab='home';try{initialTab=new URLSearchParams(location.search).get('tab')||localStorage.getItem('infoterm-active-tab')||'home';}catch(e){}if(!document.querySelector('.tab-button[data-target-tab=\"'+initialTab+'\"]')) initialTab='home';";
  page += "document.querySelectorAll('.tab-button').forEach(function(btn){btn.addEventListener('click',function(){showTab(btn.getAttribute('data-target-tab'));});});showTab(initialTab);";
  page += "var panelStorageKey='infoterm-panel-state-v1';";
  page += "document.querySelectorAll('.swatch input').forEach(function(input){";
  page += "input.addEventListener('change',function(){";
  page += "document.querySelectorAll('.swatch input[name=\"'+input.name+'\"]').forEach(function(peer){";
  page += "peer.closest('.swatch').classList.toggle('active', peer.checked);";
  page += "});";
  page += "var valueEl=document.getElementById(input.name+'-value');";
  page += "if(valueEl&&input.dataset.label){valueEl.textContent=input.dataset.label;}";
  page += "});";
  page += "});";
  page += "function readPanelState(){try{return JSON.parse(localStorage.getItem(panelStorageKey)||'{}');}catch(e){return {};}}";
  page += "function writePanelState(state){localStorage.setItem(panelStorageKey,JSON.stringify(state));}";
  page += "function applyPanelState(panel,collapsed){panel.classList.toggle('collapsed',collapsed);var btn=panel.querySelector('.panel-toggle');if(btn){btn.setAttribute('aria-expanded',collapsed?'false':'true');}}";
  page += "var savedPanelState=readPanelState();";
  page += "document.querySelectorAll('.panel[data-panel]').forEach(function(panel){";
  page += "var panelId=panel.getAttribute('data-panel');";
  page += "if(Object.prototype.hasOwnProperty.call(savedPanelState,panelId)){applyPanelState(panel,!!savedPanelState[panelId]);}";
  page += "});";
  page += "document.querySelectorAll('.panel-toggle').forEach(function(btn){";
  page += "btn.addEventListener('click',function(){";
  page += "var panel=btn.closest('.panel');";
  page += "var collapsed=!panel.classList.contains('collapsed');";
  page += "applyPanelState(panel,collapsed);";
  page += "var state=readPanelState();";
  page += "var panelId=panel.getAttribute('data-panel');";
  page += "if(panelId){state[panelId]=collapsed;writePanelState(state);}";
  page += "});";
  page += "});";
  page += "</script>";
  page += "</div></body></html>";

  htmlEnd(page);
}


// Percent-encodes a query parameter value before it is re-embedded in the
// Location header - the raw values could otherwise smuggle header/URL syntax
// (CR/LF, '&', '#') into the redirect (review finding, July 2026).
static String urlEncodeParam(const String& value) {
  static const char* hexDigits = "0123456789ABCDEF";
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      out += '%';
      out += hexDigits[((unsigned char)c) >> 4];
      out += hexDigits[((unsigned char)c) & 0x0F];
    }
  }
  return out;
}

void handleDataPointsPage() {
  String target = "/?tab=datapoints";
  if (server.hasArg("src")) {
    target += "&src=" + urlEncodeParam(server.arg("src"));
  }
  if (server.hasArg("q")) {
    target += "&q=" + urlEncodeParam(server.arg("q"));
  }
  server.sendHeader("Location", target);
  server.send(303);
}

#include "storage/StoragePersistRuntime.inc"
#include "storage/StorageBackupRuntime.inc"

void handleBackupExport() {
  if (!requireAuth()) return;
  const char* path = "/config_backup.json";
  if (!InfoTermBackupRestoreManager::exportBackup(path) || !LittleFS.exists(path)) {
    server.send(500, "text/plain", TXTW(L_WEB_EXPORT_FAILED));
    return;
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", TXTW(L_WEB_EXPORT_FAILED));
    return;
  }
  time_t nowT = time(nullptr);
  struct tm tmNow;
  localtime_r(&nowT, &tmNow);
  char stamp[20];
  strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmNow);
  String downloadName = String("InfoTerm_backup_") + stamp + ".json";
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + downloadName + "\"");
  server.streamFile(file, "application/json");
  file.close();
}

static File backupImportUploadFile;
static const char* BACKUP_IMPORT_TMP_PATH = "/config_import.json";
// A real export is a few KB; 64 KB leaves generous headroom while stopping
// an (authenticated or forged) upload from exhausting heap/filesystem.
static const size_t BACKUP_IMPORT_MAX_BYTES = 64 * 1024;
static size_t backupImportUploadBytes = 0;
static bool backupImportUploadTooLarge = false;

void handleBackupImportUpload() {
  HTTPUpload& upload = server.upload();
  // Reject the streamed body before writing anything if not authenticated
  // or without the CSRF token from the URL query (the final 401/403 is sent
  // by handleBackupImportDone()).
  if (!webAuthOk() || !csrfOk()) return;
  if (upload.status == UPLOAD_FILE_START) {
    backupImportUploadBytes = 0;
    backupImportUploadTooLarge = false;
    if (mountConfigFilesystem(false)) {
      backupImportUploadFile = LittleFS.open(BACKUP_IMPORT_TMP_PATH, "w");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (backupImportUploadTooLarge || !backupImportUploadFile) return;
    backupImportUploadBytes += upload.currentSize;
    if (backupImportUploadBytes > BACKUP_IMPORT_MAX_BYTES) {
      backupImportUploadTooLarge = true;
      backupImportUploadFile.close();
      LittleFS.remove(BACKUP_IMPORT_TMP_PATH);
      Logger::warning("Backup-Import abgelehnt: Datei groesser als 64 KB");
      return;
    }
    backupImportUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (backupImportUploadFile) {
      backupImportUploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (backupImportUploadFile) {
      backupImportUploadFile.close();
      LittleFS.remove(BACKUP_IMPORT_TMP_PATH);
    }
  }
}

void handleBackupImportDone() {
  if (!requireAuthAndCsrf()) return;
  bool ok = !backupImportUploadTooLarge &&
            InfoTermBackupRestoreManager::importBackup(BACKUP_IMPORT_TMP_PATH);
  backupImportUploadTooLarge = false;
  LittleFS.remove(BACKUP_IMPORT_TMP_PATH);
  server.sendHeader("Location", ok ? "/?tab=settings" : "/?tab=settings&importError=1");
  server.send(303);
}

// MQTT DataPoints only: export/import the DataPoint definitions without the
// rest of the config, so they can be shared between terminals (0.9.30).
// Defined in storage/StorageBackupRuntime.inc.
extern bool serializeMqttDataPointsToFile(const char* path);
extern bool deserializeMqttDataPointsFromFile(const char* path);

void handleMqttExport() {
  if (!requireAuth()) return;
  const char* path = "/mqtt_datapoints.json";
  if (!serializeMqttDataPointsToFile(path) || !LittleFS.exists(path)) {
    server.send(500, "text/plain", TXTW(L_WEB_EXPORT_FAILED));
    return;
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", TXTW(L_WEB_EXPORT_FAILED));
    return;
  }
  time_t nowT = time(nullptr);
  struct tm tmNow;
  localtime_r(&nowT, &tmNow);
  char stamp[20];
  strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmNow);
  String downloadName = String("InfoTerm_mqtt_") + stamp + ".json";
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + downloadName + "\"");
  server.streamFile(file, "application/json");
  file.close();
}

static File mqttImportUploadFile;
static const char* MQTT_IMPORT_TMP_PATH = "/mqtt_import.json";
// 32 DataPoints of typical length are ~6 KB; 16 KB is generous headroom.
static const size_t MQTT_IMPORT_MAX_BYTES = 16 * 1024;
static size_t mqttImportUploadBytes = 0;
static bool mqttImportUploadTooLarge = false;

void handleMqttImportUpload() {
  HTTPUpload& upload = server.upload();
  if (!webAuthOk() || !csrfOk()) return;
  if (upload.status == UPLOAD_FILE_START) {
    mqttImportUploadBytes = 0;
    mqttImportUploadTooLarge = false;
    if (mountConfigFilesystem(false)) {
      mqttImportUploadFile = LittleFS.open(MQTT_IMPORT_TMP_PATH, "w");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (mqttImportUploadTooLarge || !mqttImportUploadFile) return;
    mqttImportUploadBytes += upload.currentSize;
    if (mqttImportUploadBytes > MQTT_IMPORT_MAX_BYTES) {
      mqttImportUploadTooLarge = true;
      mqttImportUploadFile.close();
      LittleFS.remove(MQTT_IMPORT_TMP_PATH);
      Logger::warning("MQTT-Import abgelehnt: Datei groesser als 16 KB");
      return;
    }
    mqttImportUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (mqttImportUploadFile) {
      mqttImportUploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (mqttImportUploadFile) {
      mqttImportUploadFile.close();
      LittleFS.remove(MQTT_IMPORT_TMP_PATH);
    }
  }
}

void handleMqttImportDone() {
  if (!requireAuthAndCsrf()) return;
  bool ok = !mqttImportUploadTooLarge &&
            deserializeMqttDataPointsFromFile(MQTT_IMPORT_TMP_PATH);
  mqttImportUploadTooLarge = false;
  LittleFS.remove(MQTT_IMPORT_TMP_PATH);
  if (ok) {
    // Same post-change refresh as handleMqttDelete/handleMqttMove: the slots
    // were persisted in the storage layer, here we resync runtime state.
    InfoTermEventBusIntegration::publishConfigurationChanged("webgui", "mqtt.import");
    InfoTermEventBusIntegration::publishWidgetChanged("webgui", "mqtt.import");
    resetDataCaches();
    reloadMQTTConfig();
    registerBuiltInDataPoints();
    syncDataPointsFromRuntime();
    pageDirty = true;
    dataDirty = true;
  }
  server.sendHeader("Location", ok ? "/?tab=mqtt" : "/?tab=mqtt&mqttImportError=1");
  server.send(303);
}

static File vpnUploadFile;
static const char* VPN_UPLOAD_TMP_PATH = "/vpn_upload.conf";
// A WireGuard .conf is well under 1 KB; 8 KB is already generous.
static const size_t VPN_UPLOAD_MAX_BYTES = 8 * 1024;
static size_t vpnUploadBytes = 0;
static bool vpnUploadTooLarge = false;

void handleVpnUploadUpload() {
  HTTPUpload& upload = server.upload();
  if (!webAuthOk() || !csrfOk()) return;
  if (upload.status == UPLOAD_FILE_START) {
    vpnUploadBytes = 0;
    vpnUploadTooLarge = false;
    if (mountConfigFilesystem(false)) {
      vpnUploadFile = LittleFS.open(VPN_UPLOAD_TMP_PATH, "w");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (vpnUploadTooLarge || !vpnUploadFile) return;
    vpnUploadBytes += upload.currentSize;
    if (vpnUploadBytes > VPN_UPLOAD_MAX_BYTES) {
      vpnUploadTooLarge = true;
      vpnUploadFile.close();
      LittleFS.remove(VPN_UPLOAD_TMP_PATH);
      Logger::warning("VPN-Upload abgelehnt: Datei groesser als 8 KB");
      return;
    }
    vpnUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (vpnUploadFile) {
      vpnUploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (vpnUploadFile) {
      vpnUploadFile.close();
      LittleFS.remove(VPN_UPLOAD_TMP_PATH);
    }
  }
}

void handleVpnUploadDone() {
  if (!requireAuthAndCsrf()) return;
  bool ok = !vpnUploadTooLarge && vpnParseConfigFile(VPN_UPLOAD_TMP_PATH);
  vpnUploadTooLarge = false;
  LittleFS.remove(VPN_UPLOAD_TMP_PATH);
  if (ok) {
    persistRuntimeConfiguration();
    InfoTermEventBusIntegration::publishConfigurationChanged("webgui", "vpn.upload");
  }
  server.sendHeader("Location", ok ? "/?tab=settings" : "/?tab=settings&vpnError=1");
  server.send(303);
}

void handleVpnConnect() {
  if (!requireAuthAndCsrf()) return;
  vpnRequestConnect();
  server.sendHeader("Location", "/?tab=settings");
  server.send(303);
}

void handleVpnDisconnect() {
  if (!requireAuthAndCsrf()) return;
  vpnDisconnect();
  server.sendHeader("Location", "/?tab=settings");
  server.send(303);
}

static bool otaUploadStarted = false;

void handleOtaUploadUpload() {
  HTTPUpload& upload = server.upload();
  // Never call Update.begin()/write() for an unauthenticated or CSRF-forged
  // request - that is the firmware-takeover path, and Update.end(true) in
  // this callback already finalizes the new firmware before the done-handler
  // runs. The 401/403 is sent by handleOtaUploadDone().
  if (!webAuthOk() || !csrfOk()) return;
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadStarted = Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaUploadStarted) {
      Update.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadStarted) {
      Update.end(true);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaUploadStarted = false;
  }
}

void handleOtaUploadDone() {
  if (!requireAuthAndCsrf()) return;
  bool ok = otaUploadStarted && !Update.hasError();
  otaUploadStarted = false;
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", ok ? "OK" : "FAIL");
  if (ok) {
    // Arm the application-level OTA rollback guard (see Application.inc
    // setup()) before restarting into the new firmware - Update.end() does
    // not put the target partition into ESP-IDF's PENDING_VERIFY state
    // (confirmed live), so this NVS flag is what actually gives the new
    // firmware a bounded number of boot attempts before auto-rollback.
    Preferences otaGuardPrefs;
    otaGuardPrefs.begin("otaguard", false);
    otaGuardPrefs.putBool("pending", true);
    otaGuardPrefs.putInt("attempts", 0);
    otaGuardPrefs.end();
    delay(1000);
    ESP.restart();
  }
}


// Read-only dump of the Logger ring buffer, newest first. Behind auth (log
// lines can carry internals like SSIDs or error details) but without CSRF -
// the route mutates nothing and a cross-origin page cannot read the response.
void handleLogView() {
  if (!requireAuth()) return;
  String out;
  out.reserve(2048);
  int count = Logger::historyCount();
  for (int i = 0; i < count; i++) {
    const Logger::HistoryEntry& e = Logger::historyAt(i);
    unsigned long secs = e.ms / 1000;
    char stamp[16];
    snprintf(stamp, sizeof(stamp), "%02lu:%02lu:%02lu", secs / 3600, (secs / 60) % 60, secs % 60);
    out += stamp;
    out += " ";
    out += Logger::levelName(e.level);
    out += " ";
    out += e.message;
    out += "\n";
  }
  if (out.length() == 0) out = "(Log ist leer - Level ggf. in den Einstellungen anheben)";
  server.send(200, "text/plain; charset=utf-8", out);
}

// Tests the broker credentials currently typed into the form without
// touching the live mqttClient (separate temporary client, own client id).
// Empty password field means "use the stored password", same semantics as
// handleSave(). Blocks the cooperative main loop for the duration of the
// connect attempt (bounded by the socket timeout / TCP connect timeout) -
// accepted for a deliberately clicked test button.
void handleMqttTest() {
  if (!requireAuthAndCsrf()) return;
  String host = server.hasArg("mqttHost") ? server.arg("mqttHost") : mqttHost;
  uint16_t port = server.hasArg("mqttPort") ? (uint16_t)server.arg("mqttPort").toInt() : mqttPort;
  String user = server.hasArg("mqttUser") ? server.arg("mqttUser") : mqttUser;
  String pass = server.hasArg("mqttPass") ? server.arg("mqttPass") : String();
  host.trim();
  user.trim();
  pass.trim();
  if (host.length() == 0) host = mqttHost;
  if (port == 0) port = mqttPort;
  if (pass.length() == 0) pass = mqttPass;

  WiFiClient testNet;
  PubSubClient testClient(testNet);
  testClient.setServer(host.c_str(), port);
  testClient.setSocketTimeout(5);
  String clientId = String("InfoTermTest-") + String((uint32_t)esp_random(), HEX);
  bool ok = user.length() > 0
      ? testClient.connect(clientId.c_str(), user.c_str(), pass.c_str())
      : testClient.connect(clientId.c_str());
  int state = testClient.state();
  if (ok) testClient.disconnect();

  String msg;
  if (ok) {
    msg = String(TXTW(L_WEB_MQTT_TEST_OK)) + host + ":" + String(port) + ")";
  } else {
    switch (state) {
      case -4: msg = TXTW(L_WEB_MQTT_TEST_TIMEOUT); break;
      case -2: msg = TXTW(L_WEB_MQTT_TEST_UNREACHABLE); break;
      case 4:  msg = TXTW(L_WEB_MQTT_TEST_AUTH); break;
      case 5:  msg = TXTW(L_WEB_MQTT_TEST_UNAUTHORIZED); break;
      default: msg = String(TXTW(L_WEB_MQTT_TEST_CODE)) + String(state) + ")"; break;
    }
  }
  server.send(200, "text/plain; charset=utf-8", msg);
}


void handleMqttDelete() {
  if (!requireAuthAndCsrf()) return;
  int idx = server.hasArg("idx") ? server.arg("idx").toInt() : -1;
  if (idx >= 0 && idx < mqttValueCount) {
    for (int i = idx; i < mqttValueCount - 1; i++) {
      mqttLabels[i] = mqttLabels[i + 1];
      mqttTopics[i] = mqttTopics[i + 1];
      mqttUnits[i] = mqttUnits[i + 1];
      mqttDecimals[i] = mqttDecimals[i + 1];
      mqttValueEnabled[i] = mqttValueEnabled[i + 1];
      mqttValues[i] = mqttValues[i + 1];
      lastMqttLine[i] = "";
    }
    mqttValueCount--;
    if (mqttValueCount < 1) {
      mqttValueCount = 1;
      mqttLabels[0] = "MQTT 1";
      mqttTopics[0] = "";
      mqttUnits[0] = "";
      mqttDecimals[0] = MQTT_DECIMALS_AUTO;
      mqttValueEnabled[0] = false;
      mqttValues[0] = "--";
    }
    for (int i = mqttValueCount; i < MQTT_VALUE_COUNT; i++) {
      mqttLabels[i] = "MQTT " + String(i + 1);
      mqttTopics[i] = "";
      mqttUnits[i] = "";
      mqttDecimals[i] = MQTT_DECIMALS_AUTO;
      mqttValueEnabled[i] = false;
      mqttValues[i] = "--";
      lastMqttLine[i] = "";
    }
    sanitizeWidgetSlotsForMqttCount();
    persistRuntimeConfiguration();
    InfoTermEventBusIntegration::publishConfigurationChanged("webgui", "mqtt.delete");
    InfoTermEventBusIntegration::publishWidgetChanged("webgui", "mqtt.delete");
    resetDataCaches();
    reloadMQTTConfig();
    registerBuiltInDataPoints();
    syncDataPointsFromRuntime();
    pageDirty = true;
    dataDirty = true;
  }
  server.send(200, "text/plain", "OK");
}


void handleMqttMove() {
  if (!requireAuthAndCsrf()) return;
  int idx = server.hasArg("idx") ? server.arg("idx").toInt() : -1;
  String dir = server.hasArg("dir") ? server.arg("dir") : "";
  if (idx >= 0 && idx < mqttValueCount) {
    if (dir == "up" && idx > 0) {
      swapMqttDataPoints(idx, idx - 1);
    } else if (dir == "down" && idx < mqttValueCount - 1) {
      swapMqttDataPoints(idx, idx + 1);
    }
    persistRuntimeConfiguration();
    InfoTermEventBusIntegration::publishConfigurationChanged("webgui", "mqtt.move");
    InfoTermEventBusIntegration::publishWidgetChanged("webgui", "mqtt.move");
    resetDataCaches();
    reloadMQTTConfig();
    registerBuiltInDataPoints();
    syncDataPointsFromRuntime();
    pageDirty = true;
    dataDirty = true;
  }
  server.send(200, "text/plain", "OK");
}

// Parses the MQTT-slot rows from the save form into the given arrays and
// returns the resulting slot count. Handles the "add slot" button and
// guarantees at least one slot. Extracted from handleSave() (F-9) to keep
// that handler readable; arrays are filled in place, so the caller's
// downstream code is unchanged.
static int parseMqttValueForm(String labels[], String topics[], String units[],
                              int decimals[], bool enabled[]) {
  int postedCount = server.hasArg("mqttValueCount") ? server.arg("mqttValueCount").toInt() : mqttValueCount;
  postedCount = constrain(postedCount, 1, MQTT_VALUE_MAX);
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    labels[i] = "";
    topics[i] = "";
    units[i] = "";
    decimals[i] = MQTT_DECIMALS_AUTO;
    enabled[i] = false;
  }
  int count = 0;
  for (int i = 0; i < postedCount && count < MQTT_VALUE_COUNT; i++) {
    String label = server.hasArg(String("mqttLabel") + String(i)) ? server.arg(String("mqttLabel") + String(i)) : mqttLabels[i];
    String topic = server.hasArg(String("mqttTopic") + String(i)) ? server.arg(String("mqttTopic") + String(i)) : mqttTopics[i];
    String unit = server.hasArg(String("mqttUnit") + String(i)) ? server.arg(String("mqttUnit") + String(i)) : mqttUnits[i];
    int dec = server.hasArg(String("mqttDec") + String(i)) ? server.arg(String("mqttDec") + String(i)).toInt() : mqttDecimals[i];
    label.trim();
    topic.trim();
    unit.trim();
    labels[count] = label;
    topics[count] = topic;
    units[count] = unit;
    decimals[count] = constrain(dec, MQTT_DECIMALS_AUTO, 4);
    enabled[count] = server.hasArg(String("mqttValEn") + String(i));
    count++;
  }
  if (server.hasArg("mqttAdd") && count < MQTT_VALUE_COUNT) {
    labels[count] = "MQTT " + String(count + 1);
    topics[count] = "";
    units[count] = "";
    decimals[count] = MQTT_DECIMALS_AUTO;
    enabled[count] = false;
    count++;
  }
  if (count < 1) {
    labels[0] = "MQTT 1";
    topics[0] = "";
    units[0] = "";
    decimals[0] = MQTT_DECIMALS_AUTO;
    enabled[0] = false;
    count = 1;
  }
  return count;
}

void handleSave() {
  if (!requireAuthAndCsrf()) return;
  String newAccent = server.hasArg("accent") ? server.arg("accent") : "cyan";
  String newBorder = server.hasArg("border") ? server.arg("border") : "cyan";
  String newBg     = server.hasArg("bg") ? server.arg("bg") : "petrol";
  String newText   = server.hasArg("text") ? server.arg("text") : "standard";
  String newMenuBg = server.hasArg("menuBg") ? server.arg("menuBg") : "theme";
  String newButtonBg = server.hasArg("buttonBg") ? server.arg("buttonBg") : "theme";
  String newWidgetBg = server.hasArg("widgetBg") ? server.arg("widgetBg") : "theme";
  String newUnits  = server.hasArg("units") ? server.arg("units") : "metric";
  String newRegion = server.hasArg("region") ? server.arg("region") : "europe";
  String newLang   = server.hasArg("lang") ? server.arg("lang") : languageCode();
  String newTz     = server.hasArg("tz") ? server.arg("tz") : timezoneKey;
  String newLoc    = server.hasArg("locname") ? server.arg("locname") : locationName;
  String newNickname = server.hasArg("deviceName") ? server.arg("deviceName") : deviceName;
  int newWifiActiveIndex = server.hasArg("wifiNet") ? server.arg("wifiNet").toInt() : wifiActiveIndex;
  newWifiActiveIndex = constrain(newWifiActiveIndex, 0, WIFI_NETWORK_COUNT - 1);

  // IP configuration (0.9.31). Static mode requires syntactically valid
  // ip/gateway/subnet; any invalid field falls back to DHCP so a typo can't
  // lock the device out via a broken static address (DNS is optional).
  String newIpMode = server.hasArg("ipMode") ? server.arg("ipMode") : ipMode;
  String newStaticIp = server.hasArg("ipStatic") ? server.arg("ipStatic") : staticIp;
  String newStaticGw = server.hasArg("ipGw") ? server.arg("ipGw") : staticGateway;
  String newStaticMask = server.hasArg("ipMask") ? server.arg("ipMask") : staticSubnet;
  String newStaticDns = server.hasArg("ipDns") ? server.arg("ipDns") : staticDns;
  newStaticIp.trim();
  newStaticGw.trim();
  newStaticMask.trim();
  newStaticDns.trim();
  if (newIpMode != "static") newIpMode = "dhcp";
  if (newIpMode == "static") {
    IPAddress tmp;
    if (!tmp.fromString(newStaticIp) || !tmp.fromString(newStaticGw) || !tmp.fromString(newStaticMask)) {
      newIpMode = "dhcp";
    }
  }
  bool ipConfigChanged = (newIpMode != ipMode) || (newStaticIp != staticIp) ||
                         (newStaticGw != staticGateway) || (newStaticMask != staticSubnet) ||
                         (newStaticDns != staticDns);
  String newVpnName = server.hasArg("vpnName") ? server.arg("vpnName") : vpnName;
  bool newMqttOn = server.hasArg("mqttOn");
  bool newMqttFallback = server.hasArg("mqttFallback");
  String newMqttHost = server.hasArg("mqttHost") ? server.arg("mqttHost") : mqttHost;
  uint16_t newMqttPort = server.hasArg("mqttPort") ? (uint16_t)server.arg("mqttPort").toInt() : mqttPort;
  String newMqttUser = server.hasArg("mqttUser") ? server.arg("mqttUser") : mqttUser;
  // The form never carries the stored password (see handleRoot): an empty
  // field keeps it, the explicit checkbox clears it.
  String newMqttPass = mqttPass;
  if (server.hasArg("mqttPassClear")) {
    newMqttPass = "";
  } else if (server.hasArg("mqttPass")) {
    String postedMqttPass = server.arg("mqttPass");
    postedMqttPass.trim();
    if (postedMqttPass.length() > 0) newMqttPass = postedMqttPass;
  }
  String newMqttTopic = server.hasArg("mqttTopic") ? server.arg("mqttTopic") : mqttWeatherflowTopic;
  String newMqttLabels[MQTT_VALUE_COUNT];
  String newMqttTopics[MQTT_VALUE_COUNT];
  String newMqttUnits[MQTT_VALUE_COUNT];
  int newMqttDecimals[MQTT_VALUE_COUNT];
  bool newMqttValueEnabled[MQTT_VALUE_COUNT];
  int newMqttValueCount = parseMqttValueForm(newMqttLabels, newMqttTopics, newMqttUnits,
                                             newMqttDecimals, newMqttValueEnabled);
  HomeWidgetType newHomeSlots[HOME_SLOT_COUNT];
  String newHomeWidgetDataPointIds[HOME_SLOT_COUNT];
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    String key = String("homeSlot") + String(i);
    String currentKey = widgetSelectionKey(homeWidgetSlots[i], homeWidgetDataPointIds[i]);
    applyWidgetSelectionKey(server.hasArg(key) ? server.arg(key) : currentKey, newHomeSlots[i], newHomeWidgetDataPointIds[i]);
  }
  String newCustomTabNames[CUSTOM_TAB_COUNT];
  bool newCustomTabEnabled[CUSTOM_TAB_COUNT];
  HomeWidgetType newCustomTabWidgets[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT];
  String newCustomTabWidgetDataPointIds[CUSTOM_TAB_COUNT][TAB_WIDGET_COUNT];
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    String tabIdx = String(t);
    newCustomTabNames[t] = server.hasArg(String("tabName") + tabIdx) ? server.arg(String("tabName") + tabIdx) : customTabNames[t];
    newCustomTabEnabled[t] = server.hasArg(String("tabEnabled") + tabIdx);
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      String key = String("tab") + tabIdx + String("Slot") + String(i);
      String currentKey = widgetSelectionKey(customTabWidgets[t][i], customTabWidgetDataPointIds[t][i]);
      applyWidgetSelectionKey(server.hasArg(key) ? server.arg(key) : currentKey, newCustomTabWidgets[t][i], newCustomTabWidgetDataPointIds[t][i]);
    }
  }

  float newLat = server.hasArg("lat") ? server.arg("lat").toFloat() : LAT;
  float newLng = server.hasArg("lng") ? server.arg("lng").toFloat() : LNG;

  newLoc.trim();
  newNickname.trim();
  newMqttHost.trim();
  newMqttUser.trim();
  newMqttTopic.trim();
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    newMqttLabels[i].trim();
    newMqttTopics[i].trim();
    newMqttUnits[i].trim();
    if (newMqttLabels[i].length() == 0) newMqttLabels[i] = "MQTT " + String(i + 1);
    if (newMqttLabels[i].length() > 18) newMqttLabels[i] = newMqttLabels[i].substring(0, 18);
    if (newMqttTopics[i].length() > 120) newMqttTopics[i] = newMqttTopics[i].substring(0, 120);
    if (newMqttUnits[i].length() > 8) newMqttUnits[i] = newMqttUnits[i].substring(0, 8);
  }
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    newCustomTabNames[t].trim();
    if (newCustomTabNames[t].length() == 0) newCustomTabNames[t] = "Tab " + String(t + 1);
    if (newCustomTabNames[t].length() > 10) newCustomTabNames[t] = newCustomTabNames[t].substring(0, 10);
  }

  if (newMqttHost.length() == 0) newMqttHost = DEFAULT_MQTT_HOST;
  if (newMqttPort == 0) newMqttPort = DEFAULT_MQTT_PORT;
  if (newMqttTopic.length() == 0) newMqttTopic = DEFAULT_MQTT_TOPIC_WEATHERFLOW_OBSERVATION;

  if (newLoc.length() == 0) newLoc = "Unknown";
  if (newNickname.length() > 24) newNickname = newNickname.substring(0, 24);
  if (newUnits != "metric" && newUnits != "imperial") newUnits = "metric";
  if (newRegion != "europe" && newRegion != "us") newRegion = "europe";
  if (newLang != "de" && newLang != "en" && newLang != "fr" && newLang != "es" && newLang != "it" &&
      newLang != "ru" && newLang != "hi" && newLang != "zh") newLang = "de";
  newTz = sanitizeTimezoneKey(newTz);

  int newSleepMin = server.hasArg("sleepMin") ? server.arg("sleepMin").toInt() : sleepIntervalMin;
  sleepIntervalMin = constrain(newSleepMin, 0, 120);

  // Tab rotation / burn-in guard (1.0.2). Only the 4 offered intervals are
  // accepted; anything else falls back to 60 s. Reset the rotation timer so
  // the user gets a full interval after saving.
  tabCycleEnabled = server.hasArg("tabCycle");
  int newTabCycleSec = server.hasArg("tabCycleSec") ? server.arg("tabCycleSec").toInt() : tabCycleIntervalSec;
  if (newTabCycleSec != 30 && newTabCycleSec != 60 && newTabCycleSec != 180 && newTabCycleSec != 360) {
    newTabCycleSec = 60;
  }
  tabCycleIntervalSec = newTabCycleSec;
  lastTabCycleMs = millis();

  logLevelValue = constrain(server.hasArg("logLevel") ? server.arg("logLevel").toInt() : logLevelValue, 0, 3);
  Logger::setLevel(logLevelValue);
  widgetBordersEnabled = server.hasArg("widgetBorder");
  bool newNightMode = server.hasArg("nightMode");
  bool newRotate180 = server.hasArg("rotate180");
  bool newFlashMode = false;

  bool locationChanged =
    (fabsf(newLat - LAT) > 0.0001f) ||
    (fabsf(newLng - LNG) > 0.0001f) ||
    (newLoc != locationName);

  bool mqttConfigChanged =
    (newMqttValueCount != mqttValueCount) ||
    (newMqttOn != mqttWeatherEnabled) ||
    (newMqttFallback != mqttFallbackOpenMeteo) ||
    (newMqttHost != mqttHost) ||
    (newMqttPort != mqttPort) ||
    (newMqttUser != mqttUser) ||
    (newMqttPass != mqttPass) ||
    (newMqttTopic != mqttWeatherflowTopic);

  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    if (mqttTopics[i] != newMqttTopics[i] || mqttUnits[i] != newMqttUnits[i] || mqttDecimals[i] != newMqttDecimals[i] || mqttValueEnabled[i] != newMqttValueEnabled[i]) mqttConfigChanged = true;
  }

  bool widgetConfigChanged = false;
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    if (homeWidgetSlots[i] != newHomeSlots[i] || homeWidgetDataPointIds[i] != newHomeWidgetDataPointIds[i]) widgetConfigChanged = true;
  }
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    if (customTabNames[t] != newCustomTabNames[t] || customTabEnabled[t] != newCustomTabEnabled[t]) widgetConfigChanged = true;
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      if (customTabWidgets[t][i] != newCustomTabWidgets[t][i] || customTabWidgetDataPointIds[t][i] != newCustomTabWidgetDataPointIds[t][i]) widgetConfigChanged = true;
    }
  }

  bool wifiNetChanged = (newWifiActiveIndex != wifiActiveIndex);

  deviceName = newNickname;
  wifiActiveIndex = newWifiActiveIndex;
  ipMode = newIpMode;
  staticIp = newStaticIp;
  staticGateway = newStaticGw;
  staticSubnet = newStaticMask;
  staticDns = newStaticDns;
  vpnName = newVpnName;
  locationName = newLoc;
  LAT = newLat;
  LNG = newLng;
  unitKey = newUnits;
  regionFormatKey = newRegion;
  uiLanguage = languageFromCode(newLang);
  timezoneKey = newTz;
  flashModeEnabled = newFlashMode;
  mqttWeatherEnabled = newMqttOn;
  mqttFallbackOpenMeteo = newMqttFallback;
  mqttHost = newMqttHost;
  mqttPort = newMqttPort;
  mqttUser = newMqttUser;
  mqttPass = newMqttPass;
  mqttWeatherflowTopic = newMqttTopic;
  mqttValueCount = newMqttValueCount;
  for (int i = 0; i < MQTT_VALUE_COUNT; i++) {
    mqttLabels[i] = newMqttLabels[i];
    mqttTopics[i] = newMqttTopics[i];
    mqttUnits[i] = newMqttUnits[i];
    mqttDecimals[i] = newMqttDecimals[i];
    mqttValueEnabled[i] = newMqttValueEnabled[i];
    mqttValues[i] = "--";
    lastMqttLine[i] = "";
  }
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    homeWidgetSlots[i] = newHomeSlots[i];
    homeWidgetDataPointIds[i] = newHomeWidgetDataPointIds[i];
  }
  for (int t = 0; t < CUSTOM_TAB_COUNT; t++) {
    customTabNames[t] = newCustomTabNames[t];
    customTabEnabled[t] = newCustomTabEnabled[t];
    for (int i = 0; i < TAB_WIDGET_COUNT; i++) {
      customTabWidgets[t][i] = newCustomTabWidgets[t][i];
      customTabWidgetDataPointIds[t][i] = newCustomTabWidgetDataPointIds[t][i];
    }
  }
  sanitizeWidgetSlotsForMqttCount();

  // Apply the new color keys to their runtime globals (accentColorKey,
  // menuBgColorKey, ...) BEFORE persisting - persistRuntimeConfiguration()
  // writes those globals' current values to NVS, so calling it first would
  // always persist the previous selection instead of what was just
  // submitted. This was most visible on menuBg/buttonBg since they default
  // to "theme" and any attempt to change them away from that reverted on
  // every save.
  applyThemeByKey(newAccent, newBg);
  applyMenuBgColorByKey(newMenuBg);
  applyButtonBgColorByKey(newButtonBg);
  applyWidgetBgColorByKey(newWidgetBg);
  applyBorderColorByKey(newBorder);
  applyTextColorByKey(newText);
  applyDeviceTimezoneByKey(timezoneKey);

  // Must run after the apply*ByKey() calls above: those just reasserted the
  // day theme from the submitted color keys, which would silently cancel an
  // already-active night overlay if this save happens during a real night
  // window.
  nightModeEnabled = newNightMode;
  nightModeActive = nightModeEnabled && isNightNow();
  if (nightModeActive) applyNightModeColors();
  if (!sleepDimmed && !sleepOff) setBacklight(BL_FULL);

  // Apply a rotation change immediately - must run before
  // persistRuntimeConfiguration() (which persists the globals' current
  // values) and before the pageDirty repaint at the end of this handler,
  // which then redraws the full screen in the new orientation. The touch
  // mapping follows displayRotated180 directly (see readTouchXY()).
  if (newRotate180 != displayRotated180) {
    displayRotated180 = newRotate180;
    applyDisplayRotation();
  }

  persistRuntimeConfiguration();
  // An IP-config change needs a fresh connect so applyIpConfiguration() runs
  // again; this drops the current association, so the WebGUI session ends and
  // the device returns under its new address (noted in the UI hint).
  if ((wifiNetChanged || ipConfigChanged) && wifiEnabled) beginWiFiConnect();
  Logger::info(String("Configuration saved; log level=") + Logger::levelName(Logger::level()));
  InfoTermEventBusIntegration::publishConfigurationChanged("webgui", "save");
  if (widgetConfigChanged) {
    InfoTermEventBusIntegration::publishWidgetChanged("webgui", "save");
  }
  if (mqttConfigChanged) {
    InfoTermEventBusIntegration::publishStorageChanged("webgui", "mqtt");
  }

  if (currentPage == PAGE_TAB1 && !customTabEnabled[0]) currentPage = PAGE_HOME;
  if (currentPage == PAGE_TAB2 && !customTabEnabled[1]) currentPage = PAGE_HOME;
  if (currentPage == PAGE_TAB3 && !customTabEnabled[2]) currentPage = PAGE_HOME;

  pageDirty = true;
  dataDirty = true;

  cacheClock = "";
  cacheHomeEmpty1 = "";
  cacheHomeEmpty2 = "";
  for (int i = 0; i < HOME_SLOT_COUNT; i++) {
    cacheHomeSlots[i] = "";
  }

  lastTempText = "";
  lastRainText = "";
  lastKpText = "";
  lastKpLevelText = "";
  lastWindText = "";
  lastWindDirText = "";
  lastNextSunLabel = "";
  lastNextSunTime = "";
  lastUptimeText = "";

  if (locationChanged) resetDataCaches();
  if (mqttConfigChanged) {
    resetDataCaches();
    reloadMQTTConfig();
  }

  server.sendHeader("Location", "/");
  server.send(303);
}


void setupWebServer() {
  ensureCsrfToken();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/datapoints", HTTP_GET, handleDataPointsPage);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/mqtt/delete", HTTP_POST, handleMqttDelete);
  server.on("/mqtt/move", HTTP_POST, handleMqttMove);
  server.on("/mqtt/test", HTTP_POST, handleMqttTest);
  server.on("/mqtt/export", HTTP_GET, handleMqttExport);
  server.on("/mqtt/import", HTTP_POST, handleMqttImportDone, handleMqttImportUpload);
  server.on("/log", HTTP_GET, handleLogView);
  server.on("/backup/export", HTTP_GET, handleBackupExport);
  server.on("/backup/import", HTTP_POST, handleBackupImportDone, handleBackupImportUpload);
  server.on("/ota/upload", HTTP_POST, handleOtaUploadDone, handleOtaUploadUpload);
  server.on("/vpn/upload", HTTP_POST, handleVpnUploadDone, handleVpnUploadUpload);
  server.on("/vpn/connect", HTTP_POST, handleVpnConnect);
  server.on("/vpn/disconnect", HTTP_POST, handleVpnDisconnect);
  server.begin();
}
