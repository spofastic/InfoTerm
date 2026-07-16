# InfoTerm Architecture

## Current Baseline

InfoTerm 1.1.0 is the current stable firmware line (this line is updated
on every stable release, see the stable checklist in `AGENTS.md`).
The project is already split into focused runtime areas, but the migration
remains conservative: existing behavior is preserved while responsibilities
are gradually moved behind module boundaries and integration adapters.

## Runtime Structure

`src/InfoTerm.cpp` (a plain `.cpp` since 0.9.26, formerly `InfoTerm.ino`)
remains the firmware entry point and unity-build root. Runtime code is split
into small include files and module folders:

- `src/Application.inc` owns the shared runtime bootstrap.
- `src/display/` contains display module boundaries and display runtime helpers.
- `src/mqtt/` contains MQTT module boundaries and MQTT runtime helpers.
- `src/weather/` contains weather module boundaries and weather runtime helpers.
- `src/widgets/` contains widget module boundaries and widget runtime helpers.
- `src/webgui/` contains WebGUI module boundaries and HTTP/WebGUI helpers.
- `src/storage/` contains storage runtime helpers.
- `src/datapoints/` contains DataPoint runtime helpers.
- `src/system/` contains Scheduler, EventBus, Logger, VersionManager and system runtime helpers.
- `src/config/` contains configuration, JSON, backup/restore and firmware migration layers.

This keeps the build stable while reducing the legacy single-file surface step by
step.

## Integration Layers

The 0.9.x line introduced the current integration foundations:

- `FirmwareMigrationManager` and `FirmwareMigrationBoot`
- `BackupRestoreManager`
- `RuntimeSchedulerIntegration`
- `EventBusIntegration`

These layers connect existing runtime behavior to the newer managers without
introducing breaking changes.

Note: earlier speculative `ConfigurationManager*`/`JsonManager*` integration
layers were never wired into the runtime and were removed in 0.9.18.
Configuration persistence runs directly through the `cfgGet*`/`cfgPut*`
wrappers in `src/storage/`, and configuration JSON (de)serialization lives in
`src/storage/StorageBackupRuntime.inc`.

## Version Handling

The application version is defined only in `include/Version.h`.

Runtime-visible version labels in WebGUI, DataPoints, status output and module
metadata use `INFOTERM_APP_VERSION` or helpers from `VersionManager`. New code
must not add hard-coded application version strings.

## DataPoints

`include/DataPointManager.h` is the runtime registry for MQTT, weather and system
values. Existing widgets continue to use the stable widget configuration model,
while selected runtime values are mirrored into DataPoints for WebGUI inspection
and later generic widget binding.

## Scheduler and EventBus

`RuntimeSchedulerIntegration` defines default runtime intervals for WiFi, MQTT,
Weather, Display, Widgets, DataPoints, System and WebServer work.

`EventBusIntegration` publishes selected runtime events and is the preferred path
for future cross-module signaling. EventBus coverage is not complete yet; 0.9.8
and later versions should continue integration without adding parallel ad-hoc
notification systems.

## Logging

`src/system/Logger.h` and `src/system/Logger.cpp` provide the central logging
API. Supported levels are `ERROR`, `WARNING`, `INFO` and `DEBUG`.

New code should use:

- `Logger::error(...)`
- `Logger::warning(...)`
- `Logger::info(...)`
- `Logger::debug(...)`

The active log level is persisted with the existing runtime configuration and
can be changed through the WebGUI settings. Output goes to Serial, and the
logger additionally keeps an in-RAM ring of the last 50 emitted lines
(only lines that passed the level filter) which backs the WebGUI log view
(`/log` route, "Log" panel in the settings tab, added in 0.9.23).

## WebGUI and LittleFS

The WebGUI is fully inline: HTML, CSS and JavaScript are generated directly in
`WebGuiRuntime.inc` and sent in the HTTP response, with no separate filesystem
upload step. An earlier LittleFS-template rendering path (`data/templates/`,
`data/assets/`) existed but was never wired to an active build flag; it was
removed in 0.9.14 along with the `data/` folder.

Since 0.9.19 pages are streamed as chunked responses
(`htmlBegin`/`htmlFlush`/`htmlEnd`) instead of being built as one large
`String`, keeping peak heap during rendering at a few KB.

WebGUI security (0.9.17/0.9.20): mutating and sensitive routes require HTTP
Basic Auth (`WEBGUI_USER`/`WEBGUI_PASS` from the git-ignored
`include/Config.local.h`; read-only pages stay open) plus a per-boot CSRF
token that is embedded in the rendered pages and checked on every mutating
route - for the multipart uploads (OTA/Backup/VPN) both auth and token are
already enforced inside the upload callback, before any streamed byte is
written. Stored secrets (MQTT password) are never rendered back into the
page; an empty password field on save means "keep the stored value".

Since 0.9.9, the WebGUI groups visual configuration in the `Design` area. The
DataPoint runtime overview is a regular `Datenpunkte` tab, and old
`/datapoints` requests are redirected there for compatibility.

LittleFS itself is still used, but only by the Backup/Restore feature
(`StorageBackupRuntime.inc`, `WebGuiRuntime.inc`'s `/backup/export` and
`/backup/import` handlers) to store the exported configuration JSON file.
Direct LittleFS access should stay confined to that code. New configuration
serialization should extend the existing `serializeConfigurationToJson()` /
`deserializeConfigurationFromJson()` in `src/storage/StorageBackupRuntime.inc`
rather than adding parallel JSON handling.

## Language and Locale Handling

Visible UI labels and locale-specific terms belong in `include/Language.h`.
Code identifiers, function names and comments remain English.

Currently localized items include:

- tab labels
- widget labels
- weekday labels
- month labels
- setup/status labels
- enabled/disabled state labels
- online/offline labels
- sync labels
- date and time display helpers

## Development Direction

Architecture work should stay incremental and build-tested:

1. Prefer integrating existing modules over adding new scaffolds.
2. Reduce duplicated logic and repeated strings.
3. Continue Scheduler and EventBus adoption in existing runtime paths.
4. Keep each version compileable and documented.
5. Avoid breaking configuration, WebGUI, MQTT and widget behavior without explicit approval.
