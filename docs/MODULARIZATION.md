# Modularization Plan

InfoTerm 0.4.0 starts the conservative PlatformIO modularization phase.

## Current state

The project still keeps most firmware logic in the unity-build root
`src/InfoTerm.cpp` (a plain `.cpp` since 0.9.26 - formerly
`src/InfoTerm.ino`; the conversion needed nothing but an explicit
`#include <Arduino.h>`, proving the unity build no longer depends on the
Arduino sketch preprocessor). The Arduino/PlatformIO runtime entry points
have been moved to:

```text
src/Application.inc
```

This keeps the build in a single translation unit while separating the top-level application loop.

## Rule

One release extracts one logical area only. Every release must compile and keep the WebGUI, persistence and display behavior stable.

## Planned extraction order

1. Application runtime entry points
2. Storage and configuration helpers
3. MQTT handling
4. Weather handling
5. Display rendering
6. WebGUI rendering
7. Widget engine

## Why include modules first?

The firmware historically relied on Arduino-style automatic prototypes and
still shares many globals. Include modules allowed a safe transition; since
0.9.26 the automatic-prototype dependency is gone (plain `.cpp` root), so
the remaining constraint for extracting independent `.cpp` modules is only
the shared global state.

## 0.4.1 - Display module scaffold

Version 0.4.1 adds the first dedicated display module scaffold:

```text
include/display/DisplayModule.h
src/display/DisplayModule.cpp
```

The module is intentionally introduced as a no-op scaffold first. This keeps the
firmware behavior unchanged while establishing the target structure for later
display refactoring. Existing TFT drawing and widget rendering remain in
the main firmware file (`src/InfoTerm.cpp` since 0.9.26, historically
`src/InfoTerm.ino`) and will be moved step by step in later releases.



## 0.4.2 MQTT module scaffold

This release adds a non-invasive MQTT module boundary:

- `include/mqtt/MqttModule.h`
- `src/mqtt/MqttModule.cpp`

The existing MQTT implementation remains in the stable legacy firmware file. The new module only defines the future interface for extracting broker connection handling, subscriptions, callbacks and DataPoint updates in later releases.


## 0.4.3 Storage module scaffold

This release adds a non-invasive storage module boundary:

- `include/storage/StorageModule.h`
- `src/storage/StorageModule.cpp`

The existing Preferences/NVS, configuration persistence and LittleFS helper logic
remain in the stable legacy firmware file. The new module only defines the future
interface for extracting persistent settings, migration helpers and filesystem
access in later releases.

## 0.4.4 DataPoint module scaffold

This release adds a non-invasive DataPoint module boundary:

- `include/datapoints/DataPointModule.h`
- `src/datapoints/DataPointModule.cpp`

The existing `DataPointManager` and runtime registration/update logic remain in
the stable legacy firmware. The new module only defines the future interface for
extracting DataPoint lifecycle handling, source grouping and diagnostics in later
releases.


## 0.4.5 Widget module scaffold

This release adds a non-invasive widget module boundary:

- `include/widgets/WidgetModule.h`
- `src/widgets/WidgetModule.cpp`

The existing home widgets, custom tab widgets, widget assignment persistence and
TFT rendering remain in the stable legacy firmware file. The new module only
defines the future interface for extracting widget lifecycle handling, DataPoint
binding and renderer selection in later releases.

## 0.4.6 module scaffolds

Version 0.4.6 adds the remaining high-level module scaffolds without moving production logic yet:

- `webgui/` for future WebGUI and HTTP route handling
- `weather/` for future weather provider logic
- `wifi/` for future WiFi/network lifecycle management
- `system/` for time, NTP, display sleep, backlight and energy management

The release intentionally changes structure only. Runtime behavior remains handled by the existing stable firmware code until each responsibility can be migrated in small, testable steps.


## Version 0.5.0 - Core Migration

The first core migration phase connects the existing module boundaries to the application lifecycle.

Included core modules:

- Display
- WiFi
- System
- Weather

The release intentionally keeps the proven legacy logic in place. The module calls are currently stable lifecycle hooks and prepare the subsequent movement of concrete runtime functions into the module implementations.

System responsibilities include time, NTP, display sleep, backlight and energy management.


## 0.5.2 Data Migration Lifecycle

Storage, MQTT and DataPoint module scaffolds are now registered in the application lifecycle. This release keeps the existing legacy implementations authoritative and only establishes safe lifecycle boundaries for later extraction.


## 0.6.0 Core Runtime Migration

The first productive migration moves selected runtime functions into module-owned include files:

- `src/display/DisplayRuntime.inc`
- `src/wifi/WifiRuntime.inc`
- `src/system/SystemRuntime.inc`
- `src/weather/WeatherRuntime.inc`

These files are still included by the unity-build root (`src/InfoTerm.cpp` since 0.9.26, historically `src/InfoTerm.ino`). This keeps the build stable while reducing the main firmware file and preparing later `.cpp` extraction once module interfaces are mature.


## 0.6.1

MQTT runtime logic was extracted into `src/mqtt/MqttRuntime.inc` while preserving the existing global runtime behavior.


## 0.6.2 Storage runtime migration

Storage helper functions and persistent configuration load/save routines were moved into `src/storage/*Runtime.inc` files while preserving existing behavior.


## 0.6.3 DataPoint Runtime Migration

DataPoint source/age formatting plus built-in registration and runtime synchronization helpers were moved to `src/datapoints/DataPointRuntime.inc`. The global `DataPointManager` remains unchanged and existing behavior is preserved.


## 0.6.4 Widget Engine Migration

The sprite based widget rendering helpers were moved from `src/InfoTerm.ino` into `src/widgets/WidgetRuntime.inc`. This keeps the runtime behavior stable while reducing the main sketch and preparing the next WebGUI migration step. Widget assignments, MQTT-backed widget values and custom tab rendering remain compatible with 0.6.3 storage.


## 0.6.5 WebGUI Migration

The WebGUI runtime was moved to `src/webgui/WebGuiRuntime.inc`. This includes root/settings rendering, DataPoint diagnostics, MQTT row actions, save handling, route registration and LittleFS static/template helpers. The implementation remains include-based for this release so the existing global state and persistence behavior are preserved.


## 1.0.11 RSS-Text-Pipeline nach InfoTermLogic.h (Issue #5, Inkrement)

Die komplette RSS-Text-Aufbereitung (Entity-Decode, Tag-Strip inkl. CDATA,
UTF-8-nach-ASCII-Transliteration, Tag-Extraktion, cleanText-Pipeline) wurde
aus `src/rss/RssRuntime.inc` in `include/InfoTermLogic.h` verschoben
(`infoterm::rsstext`, Template ueber den String-Typ wie `htmlEscaped`).
Die Firmware ruft ueber duenne Wrapper exakt den Code auf, den die
Host-Tests in `test/test_logic/` abdecken (12 neue Tests: Entities
benannt/numerisch/unbekannt, Transliteration, Tag-Strip, Tag-Grenzen,
Self-Closing, fehlende Schliesser, Pipeline, Truncation).
