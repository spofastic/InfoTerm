# InfoTerm Setup Guide

This guide explains how to install and upload InfoTerm to a compatible ESP32 touchscreen board using PlatformIO. PlatformIO is the only supported build path: `platformio.ini` pins the exact library versions and the custom partition table (`partitions_infoterm.csv`) that the firmware requires. Since 0.9.26 the sources are plain `.cpp` files (no `.ino`), so the project no longer opens as an Arduino IDE sketch.

## What You Need

- A compatible ESP32 touchscreen board
- A USB cable with data support
- PlatformIO (VS Code extension or `platformio` CLI)
- Access to a Wi-Fi network
- The InfoTerm source code

## 1. Install PlatformIO

Either install the **PlatformIO IDE** extension in VS Code, or the
standalone CLI (`pip install platformio`). Board support (ESP32) and all
libraries are resolved automatically from `platformio.ini` on the first
build - no manual board or library installation is needed.

## 2. Libraries and Display Configuration

All library dependencies (`TFT_eSPI`, `ArduinoJson`, `PubSubClient`,
`XPT2046_Touchscreen`, `WireGuard-ESP32` and the ESP32 core libraries)
are pinned in `platformio.ini` and installed automatically.

The TFT_eSPI display configuration is also injected via `build_flags` in
`platformio.ini` (`USER_SETUP_LOADED` plus the pin mapping) - no manual
`User_Setup.h` editing is required for the reference board. The file
`hardware/User_Setup.h` is kept as documentation of that known-working
configuration.

If the display configuration is wrong for your board variant, you may see:

- A black or white screen
- Wrong colors
- Incorrect rotation
- No visible output
- Touch and display coordinates not matching

The included setup uses an ST7789 display with 240x320 resolution and the pin mapping documented in `hardware/User_Setup.h`.

## 3. Configure Credentials

Create the git-ignored local credentials file (see `docs/CONFIGURATION.md`):

```text
include/Config.local.h
```

It carries your real Wi-Fi networks (`WIFI_NETWORKS[]`, multiple entries
possible) and the WebGUI login (`WEBGUI_USER`/`WEBGUI_PASS`). Without it,
the placeholder values from `include/Config.h` are used. Never put real
credentials into `include/Config.h` - only `Config.local.h` is excluded
from version control.

By default the device obtains its address via DHCP. If your network has no
DHCP server, set a static address for the very first boot in
`Config.local.h` (afterwards it is editable in the WebGUI under
Settings > Netzwerk / IP-Adresse):

```cpp
#define INFOTERM_IP_MODE       "static"
#define INFOTERM_STATIC_IP     "192.168.1.50"
#define INFOTERM_STATIC_GATEWAY "192.168.1.1"
#define INFOTERM_STATIC_SUBNET "255.255.255.0"
#define INFOTERM_STATIC_DNS    "192.168.1.1"   // optional
```

## 4. Build and Upload

From the project folder:

```text
pio run -t upload
```

(or the Upload button of the PlatformIO IDE). The correct flash offset for
the custom partition table is applied automatically. IMPORTANT: on a
device that was previously flashed with a different partition layout, run
a one-time full erase first (`pio run -t erase`) - see the notes in
`platformio.ini`.

On some ESP32 boards, you may need to hold the **BOOT** button while
flashing starts.

## 5. First Boot

After upload, the device should:

- Initialize the display
- Connect to Wi-Fi
- Start the local web interface
- Load stored runtime settings if available
- Display the Start page

Open the Serial Monitor to check boot messages and find the local IP address.

## 6. Open the Web Interface

Open the device IP address in your browser.

From the web interface you can configure:

- Language
- MQTT broker settings
- MQTT values 1 to 6
- MQTT labels, topics and units
- Weather behavior
- Display and widget settings
- Status information

## 7. MQTT Setup

If you want to use MQTT values, configure:

- Broker hostname or IP
- Port
- Username and password if required
- Topic for each value
- Display label for each value
- Unit for each value

Recommended broker behavior:

- Use retained messages for values that should appear immediately after boot.
- Keep payloads simple and numeric where possible.
- Configure units in InfoTerm instead of including them in payloads.

## 8. Troubleshooting

### Display stays black or white

- Check the TFT_eSPI `build_flags` in `platformio.ini` (reference:
  `hardware/User_Setup.h`).
- Confirm the correct display driver is enabled.
- Verify display power and backlight pin.

### Colors are wrong

- Check `TFT_RGB_ORDER`.
- Some ST7789 modules require `TFT_BGR`.

### Touch does not match the display

- Verify touch controller wiring.
- Check rotation settings.
- Recalibrate if your board requires calibration.

### Upload fails

- Select the correct port.
- Use a data-capable USB cable.
- Hold the BOOT button during flashing if required.

### Wi-Fi does not connect

- Check `include/Config.local.h` (`WIFI_NETWORKS[]`).
- Verify SSID and password.
- Make sure the network is in range.
- Use Serial Monitor for connection logs.

### MQTT values do not update

- Check broker address and credentials.
- Confirm topics.
- Verify that the broker receives messages.
- Use retained messages for persistent values.

## 9. Acknowledgements

InfoTerm is inspired by and builds on ideas and implementation work from DeskBuddy by LextZip.

Original project: <https://github.com/LextZip/Deskbuddy>
