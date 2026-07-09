# Security Policy

## Supported Versions

InfoTerm is currently in its 0.9.x development line.

| Version | Supported |
| --- | --- |
| 0.9.x | Yes |

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

## Credentials

- Do not commit real Wi-Fi or MQTT credentials. `include/Config.h` holds only
  placeholders; put your real `WIFI_NETWORKS[]` and the WebGUI login
  (`WEBGUI_USER` / `WEBGUI_PASS`) in `include/Config.local.h`, which is
  git-ignored.
- The configuration backup export (`/backup/export`) deliberately omits
  secrets (WireGuard private key, MQTT password). After restoring a backup,
  re-upload the WireGuard `.conf` and re-enter the MQTT password.
