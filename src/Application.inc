#include <Update.h>
#include "esp_ota_ops.h"
#include "display/DisplayModule.h"
#include "wifi/WifiModule.h"
#include "system/SystemModule.h"
#include "weather/WeatherModule.h"
#include "storage/StorageModule.h"
#include "mqtt/MqttModule.h"
#include "datapoints/DataPointModule.h"
#include "widgets/WidgetModule.h"
#include "webgui/WebGuiModule.h"
#include "system/EventBusIntegration.h"
#include "system/RuntimeSchedulerIntegration.h"

/******************************************************************************
 * InfoTerm
 * Version : 0.6.0
 * Module  : Application runtime
 *
 * Contains the PlatformIO/Arduino setup() and loop() entry points.
 * This is the first safe modularization step: the large legacy firmware
 * remains in one compilation unit while the runtime orchestration is
 * separated into a dedicated include module.
 ******************************************************************************/

// 0.9.11 Scheduler pilot: routes the web server poll through
// RuntimeSchedulerIntegration instead of an unthrottled per-loop call.
// TaskCallback is a plain void() function pointer, so the call is wrapped here.
static void schedulerHandleWebServer() {
  server.handleClient();
}

// 0.9.11 Scheduler pilot (continued): MQTT reconnect and PubSubClient loop
// now run as scheduler tasks. ensureMQTTVerbunden() already gates actual
// reconnect attempts behind its own MQTT_RECONNECT_INTERVAL_MS (5000ms),
// matching the RuntimeTask::MQTTReconnect default interval, so this only
// changes how often the check itself runs, not the reconnect cadence.
static void schedulerEnsureMqttConnected() {
  ensureMQTTVerbunden();
}

static void schedulerMqttLoop() {
  if ((mqttWeatherEnabled || mqttDisplayHasEnabledValues()) && mqttClient.connected()) {
    mqttClient.loop();
  }
}

// 0.9.11 Scheduler pilot (continued): sun/weather/Kp refresh now runs as a
// scheduler task. The legacy interval (DATA_TICK_MS = 30000ms) is passed
// explicitly because it does not match RuntimeTask::WeatherUpdate's own
// default (60000ms) — this keeps the existing refresh cadence unchanged.
static void schedulerWeatherUpdate() {
  ensureSunTimesForToday();
  ensureWeather();
  ensureKpIndex();
  checkNightModeTransition();
}

// 0.9.11.1 Scheduler pilot (continued): DataPoint sync now runs as a
// scheduler task (1000ms default) instead of every loop iteration.
// syncDataPointsFromRuntime() has no side effects beyond setting already
// computed values, so the reduced cadence does not change behavior visibly.
static void schedulerDataPointSync() {
  syncDataPointsFromRuntime();
  InfoTermDataPoints::updateRuntimeValues();
}

// 0.9.11.1 Scheduler pilot (continued): periodic WiFi auto-reconnect.
// No existing code restarted a connection attempt after the initial boot
// connect or a manual toggle in the WebGUI, so a dropped WiFi connection
// previously stayed dropped until reboot. This starts a new attempt via the
// existing beginWiFiConnect()/updateWiFiConnectionState() pair whenever the
// device is disconnected and no attempt is already in progress.
static void schedulerWiFiReconnect() {
  if (wifiEnabled && WiFi.status() != WL_CONNECTED && !wifiConnectInProgress) {
    beginWiFiConnect();
  }
}

// 0.9.15: periodic background WiFi scan so the WebGUI network dropdown can
// show which configured networks are currently in range. Skipped entirely
// if there is nothing to differentiate (only one configured network) or
// while a VPN tunnel is active (don't disturb it with channel-hopping
// probe scans).
//
// Originally used the async WiFi.scanNetworks(true) to avoid blocking this
// cooperative scheduler tick (same lesson as the VPN connect path, see
// src/vpn/VpnRuntime.inc). Live-tested and found unusable in practice: once
// the radio has been through a connect/disconnect cycle, scanComplete()
// reliably returns WIFI_SCAN_FAILED (-2) forever afterward on this ESP32
// core version - confirmed via repeated log output, never a transient
// glitch. Switched to a synchronous scan with a shortened per-channel dwell
// time (150ms vs. the ~300ms default) to keep the actual block to roughly
// 1-2s, once every WIFI_SCAN_INTERVAL_MS - a short, infrequent hiccup was
// judged an acceptable trade-off against a scan that never works at all.
static unsigned long wifiScanLastStartMs = 0;
static bool wifiScanEverStarted = false;
static const unsigned long WIFI_SCAN_INTERVAL_MS = 30000UL;

static void schedulerWifiScan() {
  // Also skip while a connection attempt is in progress or the radio isn't
  // associated at all - scanning mid-connect risks disrupting that attempt.
  if (configuredWifiSlotCount() <= 1 || !wifiEnabled || vpnConnected) return;
  if (wifiConnectInProgress || WiFi.status() != WL_CONNECTED) return;
  if (wifiScanEverStarted && millis() - wifiScanLastStartMs < WIFI_SCAN_INTERVAL_MS) return;

  wifiScanEverStarted = true;
  wifiScanLastStartMs = millis();

  int16_t result = WiFi.scanNetworks(false, false, false, 150);
  Logger::info(String("[WiFiScan] scanNetworks() -> ") + result);

  if (result >= 0) {
    for (int i = 0; i < WIFI_SLOT_COUNT; i++) wifiNetworkReachable[i] = false;
    for (int16_t f = 0; f < result; f++) {
      String foundSsid = WiFi.SSID(f);
      for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
        if (wifiSlotSsid[i].length() > 0 && foundSsid == wifiSlotSsid[i]) wifiNetworkReachable[i] = true;
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      // The currently connected network is reachable by definition, even
      // if this particular scan pass missed it.
      String connectedSsid = WiFi.SSID();
      for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
        if (wifiSlotSsid[i].length() > 0 && connectedSsid == wifiSlotSsid[i]) wifiNetworkReachable[i] = true;
      }
    }
    wifiScanEverCompleted = true;
  }
  WiFi.scanDelete();
}

// 0.9.11.9 Scheduler pilot (continued): the periodic dynamic-content refresh
// (clock, live values, EventBus tick) now runs as a scheduler task instead of
// a manual millis()-based check in loop(). The legacy interval
// (CLOCK_TICK_MS = 1000ms) is passed explicitly because it does not match
// RuntimeTask::DisplayRefresh's own default (250ms) - this keeps the
// existing refresh cadence unchanged. The pageDirty-driven full redraw is
// deliberately NOT moved here: it must react within the same loop()
// iteration as a touch press or page change, not wait for a scheduled
// interval, so it stays in loop() untouched. RuntimeTask::WidgetRefresh is
// deliberately left unregistered - there is no separate widget-only refresh
// in the legacy code, only this same dynamic-update path.
static void schedulerDisplayRefresh() {
  InfoTermEventBusIntegration::publishSystemTick("runtime");
  updateCurrentPageDynamic();
  dataDirty = false;
}

// 0.9.14: auto-sleep dimming was still a raw per-loop-iteration millis()
// check; moved onto the previously-reserved-but-unused
// RuntimeTask::SystemStatus slot (5000ms default). handleAutoSleep() only
// compares elapsed idle time against minute-scale thresholds, so a few
// seconds of scheduling jitter before dimming/turning off the backlight is
// not noticeable.
static void schedulerSystemStatus() {
  handleAutoSleep();
  handleTabCycle();
}

void setup() {

  Logger::begin();
  Logger::info("InfoTerm boot");

  // OTA rollback guard (0.9.13, application-level - see [[feedback-esp32-ota-rollback-not-automatic]]):
  // Update.end()/esp_ota_set_boot_partition() does NOT put the newly written
  // partition into ESP-IDF's PENDING_VERIFY state (confirmed live: reading
  // esp_ota_get_state_partition() right after an OTA boot showed VALID, not
  // PENDING_VERIFY/NEW), so the bootloader's built-in auto-rollback never
  // triggers no matter how badly the new firmware crashes. This gives the
  // new firmware exactly one boot attempt to reach the confirmation point at
  // the end of setup(); if it crashes before getting there twice in a row,
  // this guard rolls back to the other (previously working) partition and
  // restarts, before any other initialization that could crash again runs.
  {
    Preferences otaGuardPrefs;
    otaGuardPrefs.begin("otaguard", false);
    if (otaGuardPrefs.getBool("pending", false)) {
      int attempts = otaGuardPrefs.getInt("attempts", 0) + 1;
      if (attempts > 1) {
        Logger::info("[OTA-Guard] new firmware failed to confirm twice, rolling back");
        otaGuardPrefs.putBool("pending", false);
        otaGuardPrefs.putInt("attempts", 0);
        otaGuardPrefs.end();
        Update.rollBack();
        ESP.restart();
      }
      otaGuardPrefs.putInt("attempts", attempts);
    }
    otaGuardPrefs.end();
  }

  pinMode(BACKLIGHT_PIN, OUTPUT);
  analogWrite(BACKLIGHT_PIN, BL_FULL);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  InfoTermStorage::begin();
  InfoTermStorage::load();
  // Loads all persisted settings from NVS (StorageLoadRuntime.inc). Formerly
  // routed through the ConfigurationManager pass-through wrapper, which added
  // no behavior and has been removed.
  loadStoredSettings();

  // Core module scaffolds are now registered in the application lifecycle.
  // Existing runtime logic remains in the legacy firmware paths and is
  // migrated gradually to keep each release stable.
  InfoTermDisplay::begin();
  InfoTermWiFi::begin();
  InfoTermSystem::begin();
  InfoTermWeather::begin();
  InfoTermDataPoints::begin();
  InfoTermMqtt::begin();
  InfoTermWidgets::begin();
  InfoTermWebGui::begin();
  InfoTermEventBusIntegration::begin();

  InfoTermRuntimeSchedulerIntegration::begin();
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::WebServerHandle,
      schedulerHandleWebServer);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::MQTTReconnect,
      schedulerEnsureMqttConnected);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::MQTTLoop,
      schedulerMqttLoop);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::WeatherUpdate,
      schedulerWeatherUpdate, DATA_TICK_MS);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::DataPointSync,
      schedulerDataPointSync);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::WiFiReconnect,
      schedulerWiFiReconnect);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::WifiScan,
      schedulerWifiScan);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::DisplayRefresh,
      schedulerDisplayRefresh, CLOCK_TICK_MS);
  InfoTermRuntimeSchedulerIntegration::registerTask(
      InfoTermRuntimeSchedulerIntegration::RuntimeTask::SystemStatus,
      schedulerSystemStatus);

  InfoTermDataPoints::registerDefaults();
  registerBuiltInDataPoints();
  syncDataPointsFromRuntime();
  InfoTermDataPoints::updateRuntimeValues();
  setBacklight(BL_FULL);
  lastInteractionMs = millis();

  delay(200);

  tft.init();
  applyDisplayRotation();  // loadStoredSettings() above already ran
  tft.invertDisplay(INV);
  tft.setSwapBytes(true);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("InfoTerm startet...", 10, 10, 2);

  // PlatformIO may install an XPT2046_Touchscreen version whose begin() method
  // does not accept an SPIClass reference. Configure the default SPI bus with
  // the touch pins and call begin() without parameters for compatibility.
  SPI.begin(T_SCK, T_MISO, T_MOSI, TOUCH_CS);
  touchSPI.begin(T_SCK, T_MISO, T_MOSI, TOUCH_CS);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  ts.begin();
  ts.setRotation(ROT);

  tft.drawString("Connecting WiFi...", 10, 34, 2);
  connectWiFi(true);
  setupMQTT();
  ensureMQTTVerbunden();

  tft.drawString(TXT(L_BOOT_SYNCING_TIME), 10, 58, 2);
  configTzTime(timezonePosixByKey(timezoneKey),
               "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  waitForNtpTime();

  ensureSunTimesForToday();
  ensureWeather();
  ensureKpIndex();
  syncDataPointsFromRuntime();

  setupWebServer();

  pageDirty = true;
  dataDirty = true;

  drawCurrentPageFull();
  updateCurrentPageDynamic();

  // OTA rollback confirmation (0.9.13): setup() reaching this point without
  // a crash/reset is the signal that this firmware is basically sound - not
  // gated on WiFi/MQTT actually connecting, since those depend on external
  // infrastructure, not firmware correctness. Clears the application-level
  // OTA guard flag (see top of setup()); esp_ota_mark_app_valid_cancel_rollback()
  // is kept too as defense in depth even though live testing showed the
  // Arduino Update library does not put partitions into ESP-IDF's
  // PENDING_VERIFY state, so that call alone does not provide real protection.
  {
    Preferences otaGuardPrefs;
    otaGuardPrefs.begin("otaguard", false);
    otaGuardPrefs.putBool("pending", false);
    otaGuardPrefs.putInt("attempts", 0);
    otaGuardPrefs.end();
  }
  esp_err_t otaValidStatus = esp_ota_mark_app_valid_cancel_rollback();
  if (otaValidStatus == ESP_OK) {
    Logger::info("[OTA] running partition marked valid, rollback cancelled");
  } else {
    Logger::info("[OTA] mark_app_valid_cancel_rollback: " + String(esp_err_to_name(otaValidStatus)));
  }
}

void loop() {
  InfoTermRuntimeSchedulerIntegration::tick();

  InfoTermMqtt::loop();
  InfoTermWiFi::update();
  InfoTermSystem::update();
  InfoTermWeather::update();
  InfoTermDisplay::update();
  InfoTermWidgets::update();
  InfoTermWebGui::update();

  updateWiFiConnectionState();

  int tx = 0, ty = 0;
  if (touchNewPress(tx, ty)) {
    lastInteractionMs = millis();
    lastTabCycleMs = millis();  // manual touch defers the next auto tab rotation

    if (sleepOff) {
      if (manualDimMode) {
        sleepOff = false;
        sleepDimmed = true;
        setBacklight(BL_DIM);
        pageDirty = true;
      } else {
        wakeDisplay();
      }
      return;
    }

    if (sleepDimmed) {
      if (!manualDimMode) {
        wakeDisplay();
      } else {
        handleNavTouch(tx, ty);
      }
    } else {
      handleNavTouch(tx, ty);
    }
  }

  if (pageDirty || lastDrawnPage != currentPage) {
    drawCurrentPageFull();
    updateCurrentPageDynamic();
    pageDirty = false;
    dataDirty = false;
  }

  delay(10);
}
