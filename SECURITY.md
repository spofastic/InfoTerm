# Security Policy

## Supported Versions

InfoTerm is currently in its 1.0.x release line.

| Version | Supported |
| --- | --- |
| 1.0.x | Yes |
| 0.9.x | No |

## Reporting a Vulnerability

Please do not publish sensitive security issues publicly before maintainers have had a chance to review them.

Report privately: use GitHub's **"Report a vulnerability"** (Security advisories) on the repository, or contact a maintainer directly. Do not open a public issue for a real vulnerability, and never post secrets, credentials or private network details.

## Threat model

InfoTerm is a single-device appliance on a local network. Read-only pages
(`/`, `/datapoints`) are intentionally reachable without a login so the
dashboard stays glanceable. All mutating/sensitive routes — configuration
save, backup export/import, OTA firmware upload, VPN upload/connect,
MQTT-slot edits, MQTT connection test and the log view — require HTTP
Basic authentication (since 0.9.17). Set a real password before exposing
the device on any shared network.

Additional hardening on top of the login:

- Every mutating route also requires a per-boot CSRF token (since 0.9.20),
  checked inside the multipart upload callbacks before any byte is written
  — a browser with cached credentials cannot be tricked into OTA/backup/
  VPN requests by a foreign page.
- Stored secrets are never rendered back into WebGUI pages; an empty
  password field on save means "keep the stored value" (since 0.9.20).
- Backup and VPN uploads enforce hard size limits (64 KB / 8 KB, since
  0.9.21); OTA is bounded by the partition size.
- All user-visible configuration values are HTML-escaped including single
  quotes (host-tested), and redirect parameters are percent-encoded.

HTTP is unencrypted: on a hostile network, Basic-Auth credentials could be
sniffed. Keep the device on a trusted LAN (or reach it through the built-in
WireGuard tunnel) and do not port-forward it to the internet.

## Known, accepted limitations

These are deliberate trade-offs for a personal appliance on a trusted LAN.
They are listed here so nobody has to rediscover them (security review,
2026-07-16); each would need to be revisited before deploying InfoTerm on a
network you do not control.

- **No TLS certificate verification on outbound fetches.** Weather, sun
  times and RSS use `WiFiClientSecure::setInsecure()` — HTTPS provides
  transport encryption but no server authenticity. A machine-in-the-middle
  can forge weather/RSS content. Mitigations in place: fetched content is
  treated as untrusted display data (size-capped streaming reads, 24 KiB
  RSS limit, ASCII transliteration, no execution/markup path), and feed
  URLs are restricted to plain http(s) without embedded credentials
  (`infoterm::isAllowedFeedUrl`, host-tested). A pinned CA or CA bundle is
  not shipped because flash is near its partition limit and an expiring
  pinned root would silently kill the weather widget.
- **OTA images are not cryptographically signed.** `Update.end(true)`
  validates image structure, not origin. Anyone holding valid WebGUI
  credentials can flash arbitrary firmware — treat the WebGUI login as
  equivalent to physical ownership of the device. The NVS rollback guard
  protects against non-booting images, not against malicious ones.
- **MQTT is plaintext.** Broker credentials and payloads are readable on
  the wire. Run the broker on the same trusted LAN segment; do not reuse
  the MQTT password anywhere else.
- **Setup portal (first boot / manual start).** While the portal is
  active, `/setup/scan` and `/setup/connect` intentionally require neither
  the WebGUI login nor a CSRF token — the gate is the WPA2 password of the
  setup access point (default `infoterm`, shown on the device display,
  overridable via `INFOTERM_AP_PASSWORD` in `Config.local.h`; the portal
  can be disabled entirely with `INFOTERM_PORTAL_ENABLED 0`). Anyone who
  joins the setup AP can configure Wi-Fi. Change the AP password in
  `Config.local.h` if that default is not acceptable in your environment,
  and close the portal (it closes itself once a station connection is
  established) rather than leaving it running.

## Credentials

- Do not commit real Wi-Fi or MQTT credentials. `include/Config.h` holds only
  placeholders; put your real `WIFI_NETWORKS[]` and the WebGUI login
  (`WEBGUI_USER` / `WEBGUI_PASS`) in `include/Config.local.h`, which is
  git-ignored.
- The configuration backup export (`/backup/export`) deliberately omits
  secrets (WireGuard private key, MQTT password). After restoring a backup,
  re-upload the WireGuard `.conf` and re-enter the MQTT password.
- Exception: RSS feed URLs are exported verbatim (full backup and
  `/rss/export`). The URL gate rejects `user:pass@host` credentials, but a
  feed URL can still carry a secret in its query or path (e.g.
  `?token=...`) — if you use such feeds, treat the backup files as
  sensitive.
