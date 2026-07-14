/******************************************************************************
 * InfoTerm
 * Version : 0.6.0
 * Module  : Wifi runtime migration
 *
 * This file contains legacy runtime functions moved out of InfoTerm.ino as the
 * first productive migration step. It is included into the legacy compilation
 * unit to keep access to the existing stable global runtime state while reducing
 * the size and complexity of the main firmware file.
 ******************************************************************************/

#include <ESPmDNS.h>

// Network hostname (DHCP option 12 + mDNS), derived from the configured
// device name so the device shows up recognizably in router overviews
// instead of the Arduino default "esp32-<MAC-suffix>" (0.9.27, user
// request). Hostname rules are stricter than the free-text device name
// (letters/digits/hyphen only, no leading/trailing hyphen), so the name is
// sanitized; an empty result falls back to "InfoTerm-<MAC-suffix>".
// A device-name change takes effect on the next reconnect/reboot.
static String networkHostname() {
  String src = deviceName;
  src.trim();
  String out;
  for (size_t i = 0; i < src.length() && out.length() < 24; i++) {
    char c = src[i];
    if (isalnum((unsigned char)c)) {
      out += c;
    } else if (out.length() > 0 && out[out.length() - 1] != '-') {
      out += '-';
    }
  }
  while (out.length() > 0 && out[out.length() - 1] == '-') {
    out.remove(out.length() - 1);
  }
  if (out.length() == 0) {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    out = "InfoTerm-" + mac.substring(mac.length() >= 4 ? mac.length() - 4 : 0);
  }
  return out;
}

// Announces <hostname>.local plus the HTTP service via mDNS so the WebGUI
// stays reachable by name even when DHCP hands out a new address (the
// trigger for this feature: the device "disappeared" after a router-side
// lease change). Called after every successful association - MDNS.end()
// first, so a reconnect or renamed device never leaves a stale announcement.
static void startMdns() {
  MDNS.end();
  String host = networkHostname();
  if (MDNS.begin(host.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Logger::info(String("[mDNS] erreichbar als ") + host + ".local");
  } else {
    Logger::warning("[mDNS] Start fehlgeschlagen");
  }
}

// Applies the persisted IP configuration to the STA interface. Must run
// after WiFi.mode(WIFI_STA) and before WiFi.begin() (0.9.31). In "static"
// mode WiFi.config() pins IP/gateway/subnet (DNS falls back to the gateway
// if left empty); any invalid field falls back to DHCP so a typo can never
// leave the device unreachable via a broken static address. In "dhcp" mode
// (and as the fallback) an all-zero WiFi.config() clears any previously
// applied static config so a static->dhcp switch actually takes effect
// (0.0.0.0 as local IP is the ESP32 core's "use DHCP" signal - INADDR_NONE
// is deliberately NOT used, it reads as a 255.255.255.255 static address on
// some core versions).
static void applyIpConfiguration() {
  const IPAddress none((uint32_t)0);
  if (ipMode == "static") {
    IPAddress ip, gw, sn, dns;
    if (ip.fromString(staticIp) && gw.fromString(staticGateway) && sn.fromString(staticSubnet)) {
      if (!dns.fromString(staticDns)) dns = gw;
      if (WiFi.config(ip, gw, sn, dns)) {
        Logger::info(String("[WiFi] Statische IP: ") + staticIp + " GW " + staticGateway);
        return;
      }
      Logger::warning("[WiFi] Statische IP-Konfiguration abgelehnt - nutze DHCP");
    } else {
      Logger::warning("[WiFi] Statische IP-Felder ungueltig - nutze DHCP");
    }
  }
  WiFi.config(none, none, none);
}

// --- Runtime Wi-Fi slot helpers (1.0.5) ---
// Connectable networks are the runtime slots wifiSlotSsid/Pass[] (seeded from
// the compile-time WIFI_NETWORKS at first boot, then editable in the WebGUI).
// Slots with an empty SSID are skipped by the connection logic.
int configuredWifiSlotCount() {
  int n = 0;
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) if (wifiSlotSsid[i].length() > 0) n++;
  return n;
}
int firstConfiguredWifiSlot() {
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) if (wifiSlotSsid[i].length() > 0) return i;
  return -1;
}
int nextConfiguredWifiSlot(int from) {
  for (int step = 1; step <= WIFI_SLOT_COUNT; step++) {
    int idx = (from + step) % WIFI_SLOT_COUNT;
    if (wifiSlotSsid[idx].length() > 0) return idx;
  }
  return -1;
}

// Starts a connection attempt against wifiSlotSsid/Pass[index], clamping to a
// valid index so a corrupted/out-of-range wifiActiveIndex can't crash this.
void startWifiAttempt(int index) {
  wifiAttemptIndex = constrain(index, 0, WIFI_SLOT_COUNT - 1);
  // ESP32's WiFi.begin() does not reliably switch to a different AP if the
  // radio is still associated with the previous one (well-known ESP32
  // Arduino core quirk) - an explicit disconnect first is required whenever
  // this is called while already connected (e.g. switching networks live
  // from the WebGUI, as opposed to the normal boot/reconnect case where the
  // radio is already idle).
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    delay(100);
  }
  // Must run before mode()/begin() so the name reaches the DHCP request
  // (ESP32 core applies the stored hostname when the STA netif starts).
  WiFi.setHostname(networkHostname().c_str());
  // Keep the setup access point alive while the onboarding portal is active so
  // connect attempts (from the portal or the reconnect scheduler) don't tear it
  // down; otherwise plain STA.
  WiFi.mode(portalActive ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  applyIpConfiguration();  // static IP or DHCP; must precede WiFi.begin()
  WiFi.begin(wifiSlotSsid[wifiAttemptIndex].c_str(), wifiSlotPass[wifiAttemptIndex].c_str());
  wifiConnectInProgress = true;
  wifiConnectStartedMs = millis();
  // Every attempt start restarts the portal's quiet window (see
  // schedulerWiFiReconnect) so the SoftAP gets PORTAL_RECONNECT_INTERVAL_MS
  // minus the attempt duration of undisturbed beacon time between retries.
  if (portalActive) portalLastAttemptMs = millis();
  Logger::info(String("[WiFi] Verbindungsversuch: ") + wifiSlotSsid[wifiAttemptIndex]);
}

void beginWiFiConnect() {
  if (!wifiEnabled) {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    wifiConnectInProgress = false;
    return;
  }

  // Start from the preferred slot, but skip it if it has no SSID (e.g. the
  // preferred index points at an empty slot). If nothing is configured at all,
  // don't attempt - Phase A (SoftAP onboarding portal) will hook in here.
  int start = constrain(wifiActiveIndex, 0, WIFI_SLOT_COUNT - 1);
  if (wifiSlotSsid[start].length() == 0) {
    int first = firstConfiguredWifiSlot();
    if (first < 0) {
      wifiConnectInProgress = false;
      return;
    }
    start = first;
  }
  wifiCycleStartIndex = start;
  startWifiAttempt(start);
}

void connectWiFi(bool waitForConnection = true) {
  beginWiFiConnect();
  if (!waitForConnection) return;

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(200);
  }
  wifiConnectInProgress = false;
  // The synchronous boot path bypasses updateWiFiConnectionState(), so the
  // mDNS announcement has to happen here as well.
  if (WiFi.status() == WL_CONNECTED) startMdns();
}

void updateWiFiConnectionState() {
  if (!wifiEnabled || !wifiConnectInProgress) return;

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiConnectInProgress = false;
    Logger::info(String("[WiFi] Verbunden: ") + wifiSlotSsid[wifiAttemptIndex] + " (" + WiFi.localIP().toString() + ")");
    startMdns();
    ensureMQTTVerbunden();
    ensureSunTimesForToday();
    ensureWeather();
    ensureKpIndex();
    // Only dataDirty (fixed 1.0.1): a reconnect is a status change shown by
    // the 1 Hz updateStatusDynamic() (partial redraw), not a reason for a
    // full drawCurrentPageFull(). pageDirty here flickered the whole screen
    // on every WiFi reconnect - and did so on the Home page too, which has no
    // connection status at all.
    dataDirty = true;
    return;
  }

  if (millis() - wifiConnectStartedMs >= WIFI_CONNECT_TIMEOUT_MS) {
    // Fallback rotation: try the next configured network before giving up
    // for this cycle. Once we would wrap back to the preferred network,
    // stop - the periodic RuntimeTask::WiFiReconnect scheduler task will
    // call beginWiFiConnect() again later and restart from the preferred
    // network.
    int nextIndex = nextConfiguredWifiSlot(wifiAttemptIndex);
    if (nextIndex >= 0 && nextIndex != wifiCycleStartIndex) {
      startWifiAttempt(nextIndex);
      return;
    }
    wifiConnectInProgress = false;
    // While the portal is active, idle the STA after a failed cycle so the
    // setup AP beacons stay on a stable channel until the next throttled
    // retry (see startSetupPortal for the full explanation).
    if (portalActive) WiFi.disconnect();
    dataDirty = true;  // status change only (see reconnect note above)
  }
}

// --- SoftAP onboarding portal (1.0.6) ---
// Opens a WPA2 setup access point + captive DNS so a phone can pick a network
// and enter its password without editing config files. Started automatically at
// boot when no configured network connects, or manually from the WebGUI.
void startSetupPortal() {
  if (portalActive) return;
  portalActive = true;
  // Give the phone a full quiet window to find and join the AP before the
  // first throttled STA retry (see schedulerWiFiReconnect).
  portalLastAttemptMs = millis();
  // The Arduino core retries a failed connect ENDLESSLY on its own:
  // NO_AP_FOUND is a "reconnectable reason" in WiFiGeneric.cpp and
  // auto-reconnect defaults to on, so after one WiFi.begin() against an
  // unreachable network the core keeps rescanning all channels forever.
  // AP+STA share one radio - those background scans drag the beacon off the
  // portal's channel and clients fail to associate even though they can see
  // the SSID. Silence the STA while the portal is up; retries happen only
  // via the throttled schedulerWiFiReconnect().
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String suffix = mac.length() >= 4 ? mac.substring(mac.length() - 4) : "";
  portalApSsid = String(INFOTERM_AP_SSID) + (suffix.length() ? ("-" + suffix) : "");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(portalApSsid.c_str(), INFOTERM_AP_PASSWORD);
  delay(100);
  IPAddress apIp = WiFi.softAPIP();
  dnsServer.start(53, "*", apIp);  // catch-all: every lookup -> the portal IP
  Logger::warning(String("[Portal] Setup-AP aktiv: ") + portalApSsid + " -> http://" + apIp.toString());
  pageDirty = true;
}

void stopSetupPortal() {
  if (!portalActive) return;
  portalActive = false;
  WiFi.setAutoReconnect(true);  // restore the normal core behavior
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  Logger::info("[Portal] beendet");
  pageDirty = true;
}

// Called once after setupWebServer() at boot: if no configured network could be
// joined, open the onboarding portal.
void maybeStartSetupPortal() {
  if (!INFOTERM_PORTAL_ENABLED) return;
  if (!wifiEnabled) return;
  if (WiFi.status() == WL_CONNECTED) return;
  startSetupPortal();
}

// Called every loop(): drive the captive DNS and auto-close the portal as soon
// as the STA side reaches a real network (the network returned, or credentials
// entered via the portal connected successfully).
void serviceSetupPortal() {
  if (!portalActive) return;
  dnsServer.processNextRequest();
  if (WiFi.status() == WL_CONNECTED) stopSetupPortal();
}

void setWifiEnabled(bool enabled) {
  wifiEnabled = enabled;
  cfgPutBool("wifiEnabled", wifiEnabled);
  markConfigSaved();
  markConfigSaved();

  if (!wifiEnabled) {
    wifiConnectInProgress = false;
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
  } else {
    beginWiFiConnect();
  }

  dataDirty = true;
  pageDirty = true;
}
