# InfoTerm

**InfoTerm** is a multilingual ESP32-based information dashboard for weather data,
MQTT values, DataPoints, widgets, device status and an inline WebGUI.

The current version is maintained centrally in `include/Version.h`; see
[docs/RELEASE_NOTES.md](docs/RELEASE_NOTES.md) and
[CHANGELOG.md](CHANGELOG.md) for the version history and per-release details.

## Hardware

ESP32 Touchscreen 2.8 inch TFT LCD display ESP-WROOM-32 ILI9341/ST7789
compatible board, commonly sold as ESP32-2432S028R.

## Features

- Touchscreen dashboard with configurable pages, widgets and custom tabs
- Multilingual user interface (German and English)
- Weather via MQTT weather station (WeatherFlow2MQTT) with Open-Meteo fallback
- Dynamic MQTT DataPoints with per-value formatting and a live DataPoints tab
- Inline WebGUI (no separate filesystem upload needed) with login,
  per-boot CSRF protection, connection test, log view and OTA progress
- Design themes with a 22-color palette, night mode and 180-degree rotation
- OTA firmware update with automatic rollback guard
- Configuration backup/restore as JSON (secrets are never exported)
- Optional on-demand WireGuard VPN client
- Multi-WiFi with automatic fallback
- Persistent configuration (NVS) that survives reflash cycles
- PlatformIO-based build with host-side unit tests and CI

## Project Structure

```text
InfoTerm/
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── CODE_OF_CONDUCT.md
├── SECURITY.md
├── LICENSE
├── platformio.ini
├── partitions_infoterm.csv
├── include/
├── src/
├── hardware/
├── docs/
├── test/
└── tools/
```

Project documentation lives in `docs/`. Version history lives in `CHANGELOG.md`
and `docs/RELEASE_NOTES.md`.

## Quick Start with PlatformIO

1. Open the `InfoTerm` folder in VS Code.
2. Install the PlatformIO IDE extension.
3. Create `include/Config.local.h` (git-ignored) with your local Wi-Fi networks and the WebGUI login (`WEBGUI_USER` / `WEBGUI_PASS`) — see `docs/CONFIGURATION.md`. `include/Config.h` keeps placeholders only.
4. Build and upload the firmware:

```bash
platformio run --environment esp32_2432s028r -t upload
```

5. Open the device IP address in your browser.

## Build Check

```bash
platformio run --environment esp32_2432s028r
```

## Documentation

- [Setup Guide](docs/SETUP_GUIDE.md)
- [Configuration](docs/CONFIGURATION.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Modularization](docs/MODULARIZATION.md)
- [MQTT](docs/MQTT.md)
- [Language System](docs/LANGUAGE.md)
- [Supported Hardware](docs/SUPPORTED_HARDWARE.md)
- [Release Notes](docs/RELEASE_NOTES.md)
- [Credits](docs/CREDITS.md)

## Versioning

The application version is maintained only in `include/Version.h`. WebGUI,
DataPoints, status output and module version labels must use the central version
macros instead of hard-coded app versions.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
