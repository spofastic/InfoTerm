# Configuration

InfoTerm separates first-boot configuration from runtime settings.

## First-Boot Wi-Fi Configuration

Real credentials belong in a git-ignored local file, not in the committed
`include/Config.h` (which holds only placeholders). Before the first flash,
create:

```text
include/Config.local.h
```

`include/Config.h` includes it automatically via `__has_include` when present
and falls back to placeholder values otherwise. Put your local Wi-Fi networks
(and the WebGUI login) there:

```cpp
#pragma once

// One or more networks; the WebGUI lets you pick a preferred one and the
// runtime falls back to the others in order.
static const WifiCredential WIFI_NETWORKS[] = {
  {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"},
};

// WebGUI login for mutating/sensitive routes. Empty WEBGUI_PASS disables it.
#define WEBGUI_USER "admin"
#define WEBGUI_PASS "change-me"
```

`include/Config.local.h` is listed in `.gitignore`, so real credentials never
reach the repository.

## WebGUI Authentication

Read-only pages (`/`, `/datapoints`) are reachable without a login. All
mutating/sensitive routes — save, backup export/import, OTA upload, VPN
upload/connect, MQTT-slot edits — require the HTTP Basic login from
`WEBGUI_USER` / `WEBGUI_PASS`. Set a real password before exposing the device
on a shared network.

## Runtime Settings

After the first boot, InfoTerm stores runtime settings in ESP32 Preferences. These values can be changed through the web interface.

Runtime settings include, depending on the firmware state:

- MQTT broker settings
- MQTT value labels
- MQTT topics
- MQTT units
- Weather settings
- Display and widget settings
- Language selection

## Backup / Restore and Secrets

The configuration backup export (`/backup/export`) deliberately omits secrets
(WireGuard private key, MQTT password). After restoring a backup, re-upload
the WireGuard `.conf` and re-enter the MQTT password. A restore never
overwrites an existing secret with an empty value.

## Public Repository Recommendation

Keep `include/Config.h` with placeholder values only; all real credentials
live in the git-ignored `include/Config.local.h`.
