// InfoTerm VPN (WireGuard) on-demand runtime
//
// Manual connect/disconnect only, triggered from the WebGUI - no auto-connect
// at boot, no persisted "was connected" flag. This library (ciniml/
// WireGuard-ESP32) has no AllowedIPs-based route scoping: once connected,
// the tunnel becomes the sole default route and the device is unreachable
// on the local LAN until disconnected - that is expected/intended here, not
// a bug (see docs/ROADMAP.md "Evaluiert, zurueckgestellt" for the earlier
// evaluation and why WebGUI-must-stay-reachable was dropped as a
// requirement).
//
// What is NOT acceptable, and is specifically guarded against below: a
// *failed* connection attempt leaving the device stranded the same way a
// successful one does (confirmed live during that evaluation - see memory
// feedback-esp32-wireguard-breaks-lan.md). Mitigated here by (1) resolving
// the endpoint hostname ourselves before ever calling wg.begin(), so the
// library's own internal DNS retry path - the one that left routing broken
// on failure - is never exercised, and (2) verifying the tunnel actually
// carries traffic after begin() succeeds, calling wg.end() automatically if
// it doesn't.
//
// wg.begin()/wg.end() and the endpoint resolution can take several seconds;
// running that synchronously inside a WebGUI request handler would freeze
// the entire device (display, touch, MQTT) since InfoTerm's scheduler runs
// cooperatively on the main loop() thread, not as separate FreeRTOS tasks.
// Connect therefore runs in its own one-shot FreeRTOS task; disconnect
// (wg.end() alone) is fast enough to stay synchronous.

#include <LittleFS.h>
#include <WireGuard-ESP32.h>

static WireGuard vpnWg;
static bool vpnConnectTaskRunning = false;

bool vpnParseConfigFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  String section = "";
  String newPrivateKey, newPeerPublicKey, newEndpointHost, newLocalAddress, newDns, newAllowedIps;
  uint16_t newEndpointPort = 51820;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#") || line.startsWith(";")) continue;
    if (line.startsWith("[")) {
      section = line;
      continue;
    }
    int eq = line.indexOf('=');
    if (eq < 0) continue;
    String key = line.substring(0, eq);
    String value = line.substring(eq + 1);
    key.trim();
    value.trim();

    if (section.startsWith("[Interface")) {
      if (key == "PrivateKey") newPrivateKey = value;
      else if (key == "Address") {
        int slash = value.indexOf('/');
        newLocalAddress = slash >= 0 ? value.substring(0, slash) : value;
      } else if (key == "DNS") {
        newDns = value;
      }
    } else if (section.startsWith("[Peer")) {
      if (key == "PublicKey") newPeerPublicKey = value;
      else if (key == "AllowedIPs") newAllowedIps = value;
      else if (key == "Endpoint") {
        int colon = value.lastIndexOf(':');
        if (colon >= 0) {
          newEndpointHost = value.substring(0, colon);
          newEndpointPort = (uint16_t)value.substring(colon + 1).toInt();
        } else {
          newEndpointHost = value;
        }
      }
    }
  }
  file.close();

  if (newPrivateKey.length() == 0 || newPeerPublicKey.length() == 0 ||
      newEndpointHost.length() == 0 || newLocalAddress.length() == 0) {
    return false;
  }

  vpnPrivateKey = newPrivateKey;
  vpnPeerPublicKey = newPeerPublicKey;
  vpnEndpointHost = newEndpointHost;
  vpnEndpointPort = newEndpointPort;
  vpnLocalAddress = newLocalAddress;
  vpnDns = newDns;
  vpnAllowedIps = newAllowedIps;
  if (vpnName.length() == 0) vpnName = newEndpointHost;
  return true;
}

// Tries to reach a host inside the tunnel network to confirm the tunnel is
// actually carrying traffic - wg.begin()/is_initialized() only confirm the
// interface was set up, not that the handshake with the peer succeeded.
static bool vpnVerifyReachable() {
  if (vpnDns.length() == 0) return vpnWg.is_initialized();
  WiFiClient client;
  client.setTimeout(3000);
  bool ok = client.connect(vpnDns.c_str(), 53);
  if (ok) client.stop();
  return ok;
}

static void vpnConnectBlocking() {
  IPAddress localIp;
  if (!localIp.fromString(vpnLocalAddress)) {
    vpnStatusMessage = "Ungueltige Tunnel-Adresse";
    return;
  }

  IPAddress endpointIp;
  bool resolved = false;
  for (int attempt = 0; attempt < 3 && !resolved; attempt++) {
    if (attempt > 0) delay(1500);
    resolved = WiFi.hostByName(vpnEndpointHost.c_str(), endpointIp);
  }
  if (!resolved) {
    vpnStatusMessage = "Endpunkt konnte nicht aufgeloest werden";
    Logger::error("[VPN] Endpunkt-Aufloesung fehlgeschlagen: " + vpnEndpointHost);
    return;
  }

  bool ok = vpnWg.begin(localIp, vpnPrivateKey.c_str(), endpointIp.toString().c_str(),
                        vpnPeerPublicKey.c_str(), vpnEndpointPort);
  if (!ok) {
    vpnStatusMessage = "Verbindung fehlgeschlagen";
    return;
  }

  bool verified = false;
  for (int attempt = 0; attempt < 5 && !verified; attempt++) {
    delay(3000);
    verified = vpnVerifyReachable();
  }
  if (!verified) {
    vpnWg.end();
    vpnStatusMessage = "Tunnel-Verifikation fehlgeschlagen";
    Logger::error("[VPN] Tunnel kam nicht zustande, wg.end() aufgerufen");
    return;
  }

  vpnConnected = true;
  vpnConnectedSinceMs = millis();
  vpnStatusMessage = "Verbunden";
  Logger::info("[VPN] Tunnel verbunden: " + vpnName);
}

static void vpnConnectTask(void* param) {
  vpnConnectBlocking();
  vpnConnectTaskRunning = false;
  vTaskDelete(nullptr);
}

// Kicks off a connection attempt in the background and returns immediately -
// see file header for why this must not block the main loop. Status updates
// asynchronously (vpnStatusMessage/vpnConnected); the WebGUI/device Info
// page picks it up on their next refresh.
void vpnRequestConnect() {
  if (vpnConnected || vpnConnectTaskRunning) return;
  if (vpnPrivateKey.length() == 0 || vpnPeerPublicKey.length() == 0 || vpnEndpointHost.length() == 0) {
    vpnStatusMessage = "Keine VPN-Konfiguration hochgeladen";
    return;
  }
  vpnConnectTaskRunning = true;
  vpnStatusMessage = "Verbindung wird aufgebaut...";
  xTaskCreate(vpnConnectTask, "vpnConnect", 8192, nullptr, 1, nullptr);
}

void vpnDisconnect() {
  if (!vpnConnected) return;
  vpnWg.end();
  vpnConnected = false;
  vpnStatusMessage = "Getrennt";
  Logger::info("[VPN] Tunnel getrennt");
}

String vpnStatusText() {
  return vpnConnected ? "Online" : "Offline";
}
