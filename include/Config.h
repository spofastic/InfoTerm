/******************************************************************************
 * InfoTerm
 * Version : 1.0.4
 * Module  : Initial configuration
 *
 * This file contains only first-boot defaults that are required before the
 * web interface can be reached. Runtime configuration is stored in ESP32
 * Preferences and can be changed from the InfoTerm web interface.
 *
 * SECRETS: Do not put real Wi-Fi credentials here. Create include/Config.local.h
 * (git-ignored) with your real WIFI_NETWORKS. When present it overrides the
 * placeholders below, so the committed repository never carries real secrets.
 ******************************************************************************/
#pragma once

// First-boot Wi-Fi credentials. Multiple networks may be listed; the WebGUI
// lets the user pick a preferred one (persisted as wifiActiveIndex), and the
// runtime falls back to the remaining entries in order if the preferred
// network is not reachable.
struct WifiCredential {
  const char* ssid;
  const char* pass;
};

// INFOTERM_PUBLIC_BUILD (1.0.7, set by the esp32_2432s028r_public PlatformIO
// env) ignores Config.local.h entirely, so a release binary for GitHub never
// carries local secrets: no Wi-Fi seeds (first boot opens the SoftAP setup
// portal) and only the documented default WebGUI login below.
#if !defined(INFOTERM_PUBLIC_BUILD) && __has_include("Config.local.h")
  // Real credentials live in the git-ignored Config.local.h (see header).
  #include "Config.local.h"
#else
  // No Wi-Fi seeds: since 1.0.6 the SoftAP onboarding portal handles the
  // first-boot setup, so builds without Config.local.h need no credentials
  // here at all (create include/Config.local.h if you want pre-seeded ones).
  static const WifiCredential WIFI_NETWORKS[] = {
    {"", ""},
  };
#endif

static const int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// Number of editable runtime Wi-Fi slots exposed in the WebGUI (1.0.5). The
// compile-time WIFI_NETWORKS above are only first-boot seeds; the runtime
// credentials live in wifiSlotSsid[]/wifiSlotPass[] (NVS) and can be changed
// from the web interface. At least 3 slots, and never fewer than the number
// of seed networks so none are lost.
static const int WIFI_SLOT_COUNT = (WIFI_NETWORK_COUNT > 3) ? WIFI_NETWORK_COUNT : 3;

// First-boot IP configuration. Default is automatic (DHCP). For a network
// without a DHCP server, set these in include/Config.local.h so the device
// comes up reachable on the very first boot; afterwards they are editable in
// the WebGUI and stored in NVS. Placed after the Config.local.h include so
// local overrides win. INFOTERM_IP_MODE is "dhcp" or "static".
#ifndef INFOTERM_IP_MODE
  #define INFOTERM_IP_MODE "dhcp"
#endif
#ifndef INFOTERM_STATIC_IP
  #define INFOTERM_STATIC_IP ""
#endif
#ifndef INFOTERM_STATIC_GATEWAY
  #define INFOTERM_STATIC_GATEWAY ""
#endif
#ifndef INFOTERM_STATIC_SUBNET
  #define INFOTERM_STATIC_SUBNET "255.255.255.0"
#endif
#ifndef INFOTERM_STATIC_DNS
  #define INFOTERM_STATIC_DNS ""
#endif

// SoftAP onboarding portal (1.0.6). If no configured Wi-Fi network connects at
// boot, the device opens a WPA2-protected setup access point so a phone can
// pick a network and enter its password without editing config files. The AP
// name and password are shown on the device display. All overridable in
// include/Config.local.h. INFOTERM_AP_PASSWORD must be >= 8 chars (WPA2).
#ifndef INFOTERM_PORTAL_ENABLED
  #define INFOTERM_PORTAL_ENABLED 1
#endif
#ifndef INFOTERM_AP_SSID
  #define INFOTERM_AP_SSID "InfoTerm-Setup"
#endif
#ifndef INFOTERM_AP_PASSWORD
  #define INFOTERM_AP_PASSWORD "infoterm"
#endif

// WebGUI authentication (HTTP Basic) for mutating/sensitive routes (/save,
// /backup/*, /ota/upload, /vpn/*, /mqtt/delete, /mqtt/move, /restart).
// Since 1.0.7 these are only FIRST-BOOT SEEDS: the runtime credentials live
// in NVS (webUser/webPass) and are editable in the WebGUI settings tab.
// The documented public-build default is admin/infoterm - change it in the
// WebGUI on any device that is actually reachable on your network. An empty
// password disables the login gate entirely.
#ifndef WEBGUI_USER
  #define WEBGUI_USER "admin"
#endif
#ifndef WEBGUI_PASS
  #define WEBGUI_PASS "infoterm"
#endif
