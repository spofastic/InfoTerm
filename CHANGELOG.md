# Changelog

Alle nennenswerten Änderungen an InfoTerm werden in dieser Datei dokumentiert.

Das Projekt orientiert sich am Format „Keep a Changelog“.

## [1.1.0] — Stable

Härtungs-Release nach externem Software-Review (2026-07-16): keine neuen
Funktionen, vier Review-Findings plus zwei Nacharbeiten aus der
Review-Nachprüfung umgesetzt. (Die Entwicklungsversion lief zunächst als
1.0.13 und wurde vor dem ersten Commit auf Wunsch des Maintainers in
1.1.0 umbenannt — 1.0.13 wurde nie veröffentlicht.)

### Fixed

- **VPN: fehlgeschlagener Task-Start hinterlässt keinen hängenden Status
  mehr.** `vpnRequestConnect()` prüft jetzt den Rückgabewert von
  `xTaskCreate()` (wie der MQTT-Verbindungstest seit 1.0.11): schlägt der
  Task-Start mangels Speicher fehl, werden `vpnConnectTaskRunning` und die
  Statusmeldung zurückgesetzt statt dauerhaft „Verbindung wird
  aufgebaut..." zu melden.

### Changed

- **RSS-Feed-URLs: eine zentrale, host-getestete Validierung.** Neues
  Prädikat `infoterm::isAllowedFeedUrl()` in `include/InfoTermLogic.h`
  (Issue-#5-Muster): nur `http://`/`https://`, Host darf nicht leer sein,
  URLs mit eingebetteten Zugangsdaten (`user:pass@host`) werden abgelehnt.
  Ersetzt die bisher dreifach duplizierte `startsWith`-Prüfung
  (Formular-Speichern, NVS-Load/`sanitizeGeneralConfig`, RSS-Import) und
  wird zusätzlich direkt vor dem Abruf in `fetchRssFeed()` durchgesetzt —
  damit ist auch der Voll-Backup-Import abgedeckt. Nach der
  Review-Nachprüfung ergänzt: Null-Guard und Kurz-Präfix-Tests (`"h"`,
  `"http"`, `"http:/"`, …), die das Short-Circuit-Verhalten der
  Präfix-Prüfung dokumentieren. 4 neue native Tests (36 gesamt).
- **Backup-Doku zu Feed-URLs präzisiert.** Feed-URLs werden unverändert
  exportiert und können selbst Secrets tragen (`?token=…`); der zu
  absolute Kommentar „no secrets involved" im Backup-Export wurde
  korrigiert und SECURITY.md um den Hinweis ergänzt, solche Backups als
  sensibel zu behandeln.
- **Dokumentierte Versionsstände repariert.** `docs/DEVELOPMENT_CONTEXT.md`
  (stand auf 1.0.0) und `docs/ARCHITECTURE.md` (stand auf 0.9.31) nennen
  wieder die tatsächliche Baseline; beide Dateien sind jetzt Teil der
  Stable-Checkliste in `AGENTS.md`, damit die Drift nicht wiederkehrt.
- **SECURITY.md beschreibt das reale Sicherheitsmodell.** Neuer Abschnitt
  „Known, accepted limitations": keine TLS-Zertifikatsprüfung bei
  Wetter/RSS (`setInsecure`), unsignierte OTA-Images (WebGUI-Login =
  Gerätehoheit), Klartext-MQTT und das Pairing-Modell des Setup-Portals
  (WPA2-Passwort des AP als einziges Gate) sind jetzt als bewusste
  Trade-offs dokumentiert. Supported-Versions-Tabelle von 0.9.x auf 1.0.x
  aktualisiert.

## [1.0.12] — Stable

### Added

- **Vierter individueller Tab (Issue #9).** `CUSTOM_TAB_COUNT` von 3 auf 4:
  „Tab 4" erscheint in der WebGUI-Tab-Konfiguration (Standard: inaktiv)
  und kann wie die anderen Tabs Widgets oder einen RSS-Feed anzeigen. Die
  Display-Nav-Bar fasst mit allen vier Tabs sechs Buttons à 40 px; zu
  lange Tab-Namen werden dafür nur auf dem Display auf die Button-Breite
  gekürzt — die WebGUI zeigt weiterhin den vollen Namen. NVS-Keys,
  Backup/Restore, Design-&-Tabs-Export und Tab-Rotation wachsen
  automatisch mit; alte Backups mit 3 Tabs bleiben importierbar (Tab 4
  behält dann seine Standardwerte).

### Changed

- **Tab-Rotation und URL-Host-Ermittlung host-testbar (Issue #5,
  Inkrement).** Der Burn-in-Rotationsschritt (`nextCyclePageId`) und die
  Host-Extraktion für Feed-Labels (`urlHost`) liegen jetzt in
  `include/InfoTermLogic.h` mit 7 neuen nativen Unit-Tests — die Rotation
  ist damit für den Sprung auf 6 Nav-Seiten abgesichert. Interne
  if-Ketten über die Tab-Seiten wurden durch Schleifen über
  `CUSTOM_TAB_COUNT` ersetzt.

## [1.0.11] — Stable

### Added

- **Export/Import von Design- und Tab-Einstellungen (Issue #8).** Auf dem
  Design-Tab gibt es einen neuen Block „Design & Tabs exportieren /
  importieren": exportiert werden ausschließlich die Design-Einstellungen
  (alle Farben, Nachtmodus/Erzwingen/Nacht-Helligkeit, Widget-Rahmen,
  Widget-Schriftgröße, Rotation), die Widget-Belegung der Startseite und
  die Konfiguration der Custom-Tabs
  (Name, Aktiv, Inhalt, Feed-Auswahl, Widget-Belegung) — z. B. um mehrere
  InfoTerms gleich aussehen zu lassen. DataPoints, RSS-Feed-URLs und alles
  auf dem Einstellungen-Tab (Netzwerk, MQTT, Standort, …) sind bewusst
  nicht enthalten; Verweise auf nicht vorhandene DataPoints/Feeds fallen
  auf dem Zielgerät sauber zurück. Eigener Dateityp-Marker
  (`infoterm-design-tabs`), sodass ein Voll-Backup oder eine
  DataPoints-Datei beim Import abgelehnt wird (Muster des
  MQTT-DataPoints-Exports: `GET /design/export`, `POST /design/import` mit
  Login + CSRF und 16-KB-Limit). Der Import wirkt sofort (Farben,
  Rotation, Nachtmodus), wird persistiert und in allen 8 Sprachen
  beschriftet.

### Added

- **RSS-Feed-Liste exportieren/importieren.** Neues „Backup"-Panel auf dem
  RSS-Tab, analog zum DataPoints-Export: `GET /rss/export` liefert die
  Feed-URLs als JSON (`InfoTerm_rss_<Zeitstempel>.json`, eigener
  Dateityp-Marker `infoterm-rss-feeds`), `POST /rss/import` (Login + CSRF,
  16-KB-Limit) ersetzt die gespeicherte Liste, setzt den Abruf-Status
  zurück und klemmt Tab-Verweise auf die neue Liste; fremde Dateien
  (Voll-Backup, DataPoints, Design) werden abgelehnt. Welcher Tab welchen
  Feed zeigt, bleibt Teil des Design-&-Tabs-Exports.

### Changed

- **Design-Tab neu gegliedert.** Die Einstellungen stehen jetzt unter vier
  fetten Gruppen-Überschriften — Anzeige (Drehung), Nachtmodus
  (Automatik/Erzwingen/Helligkeit), Widgets (Rahmen/Schriftgröße), Farben
  (Paletten) — statt einzelner Labels pro Schalter; der
  Design-&-Tabs-Export sitzt in einem eigenen aufklappbaren
  „Backup"-Panel unter dem Design-Panel.
- **MQTT-Tab: DataPoints-Export in eigenes „Backup"-Panel** unter der
  DataPoints-Tabelle verschoben (gleiche Darstellung wie auf dem
  Design-Tab), statt als Block in der Tabelle zu hängen.
- **MQTT-Verbindungstest blockiert die Oberfläche nicht mehr (Issue #3).**
  Der Verbindungsversuch läuft jetzt in einem eigenen FreeRTOS-Task (Muster
  des VPN-Connect-Pfads) statt im WebGUI-Handler — Display, Touch und
  WebGUI bleiben während der bis zu ~5 s dauernden Prüfung bedienbar. Die
  Seite pollt `GET /mqtt/test/status` (202 solange der Test läuft, 200 mit
  der gewohnten lokalisierten Meldung danach); Optik und Meldungstexte
  sind unverändert.
- **RSS-Text-Aufbereitung in host-testbare Einheit extrahiert (Issue #5,
  Inkrement).** Entity-Decode, Tag-Strip, UTF-8-Transliteration,
  Tag-Extraktion und die cleanText-Pipeline liegen jetzt als
  `infoterm::rsstext` in `include/InfoTermLogic.h`; die Firmware ruft über
  dünne Wrapper exakt den Code auf, den 12 neue native Unit-Tests
  (`test/test_logic/`) abdecken. Verhalten unverändert; Details in
  `docs/MODULARIZATION.md`.
- **„Aktiv"-Haken der Custom-Tabs in die Kopfzeile verschoben.** Die
  Checkbox sitzt jetzt oben neben der Tab-Überschrift statt unten im
  Formular-Raster neben der Feed-Auswahl; ein Klick auf den Haken klappt
  das Panel nicht ein.
- **WebGUI-Tab „RSS Feed" heißt jetzt „RSS"** und ist im Menü nach links
  vor MQTT gerückt (Reihenfolge: Home, Custom-Tabs, RSS, MQTT,
  DataPoints, Design, Einstellungen).

## [1.0.10] — Stable

### Added

- **RSS-Feeds auf Custom-Tabs (Issue #7).** Neuer WebGUI-Tab „RSS Feed"
  zwischen DataPoints und Design: dort lassen sich bis zu 4 Feed-URLs
  pflegen (Editor nach dem Muster der MQTT-DataPoints-Tabelle, mit
  Status-Spalte und Löschen-Button). In der Tab-Konfiguration wählt ein
  neues Dropdown „Inhalt" zwischen dem Widget-Raster („DataPoints") und
  „RSS-Feed"; daneben bestimmt ein Feed-Dropdown, welcher der
  gespeicherten Feeds angezeigt wird. Das Display zeigt immer nur den
  neuesten Eintrag des Feeds (Kanaltitel, Datum, Titel, Text); ist der
  Eintrag höher als der Bildschirm, scrollt er langsam pixelweise nach
  oben (Pause an Anfang und Ende, dann von vorn). Abruf alle 10 Minuten
  pro referenziertem Feed (RSS 2.0 und Atom, HTTP und HTTPS), höchstens
  ein Feed pro Scheduler-Tick, Leselimit 24 KB. HTML-Tags werden
  entfernt, Entities dekodiert und Unicode für die Display-Fonts nach
  ASCII transliteriert (ae/oe/ue/ss). Feed-Liste und Tab-Auswahl liegen
  in NVS (`rssUrl0..3`, `tabContent<t>`, `tabFeed<t>`) und sind Teil von
  Backup/Restore. Neue Sprachstrings in allen 8 Sprachen.

## [1.0.9] — Stable

### Added

- **Wählbare Widget-Schriftgröße.** Neues Dropdown „Widget-Schriftgröße"
  (Klein / Normal / Groß) im Design-Tab (Issue #1). „Klein" zeichnet die
  Widget-Werte in 16 px (Font 2), „Normal" wie bisher in 26 px (Font 4),
  „Groß" in 32 px (Font 2 mit 2×-Skalierung); Werte, die für „Groß" zu breit
  für die 108-px-Karte sind, fallen automatisch pro Widget auf Normal zurück,
  statt am Kartenrand abgeschnitten zu werden. Das Layout (Anzahl und Größe
  der Widgets) bleibt unverändert; die Uhr-Karte ist nicht betroffen. Der
  Wert wird in NVS persistiert (`widgetFont`) und ist Teil von
  Backup/Restore. Neue Sprachstrings in allen 8 Sprachen.
- **Freier Heap als System-DataPoint.** Neuer Datenpunkt `system.freeHeap`
  (Issue #2) zeigt den freien Heap in KB im DataPoints-Tab und ist als Widget
  auf Home/Tabs wählbar — für die Langzeitbeobachtung des Speicherverbrauchs.

## [1.0.8] — Stable

### Added

- **Wählbare Nacht-Helligkeit.** Im Design-Tab unter dem Nachtmodus-Schalter
  gibt es ein neues Dropdown „Nacht-Helligkeit" (10 / 25 / 50 / 75 / 100 %,
  Standard 25 %). Solange die Nachtpalette aktiv ist, wird das
  Display-Backlight per PWM auf diesen Wert gedimmt (`awakeBacklightLevel()`);
  beim Übergang zurück zum Tag, beim Aufwachen aus dem Sleep und nach dem
  Speichern in der WebGUI wird der jeweils passende Wert gesetzt. Die
  Sleep-Stufen (Dimmen/Aus) bleiben unverändert und greifen weiterhin
  zusätzlich. Der Wert wird in NVS persistiert (`nightBrightPct`) und ist
  Teil von Backup/Restore. Neue Sprachstrings in allen 8 Sprachen.
- **Schalter „Nachtmodus erzwingen".** Neuer Schalter im Design-Tab zwischen
  Nachtmodus und Nacht-Helligkeit: aktiviert die Nachtpalette samt Dimmung
  sofort und unabhängig vom Sonnenauf-/-untergangsfenster — auch tagsüber,
  z. B. zum Testen der Nachtmodus-Einstellungen oder für dauerhaft dunklen
  Betrieb. Wirkt unabhängig vom Automatik-Häkchen (ein Schalter genügt),
  wird persistiert (`nightForce`, überlebt Reboots) und ist Teil von
  Backup/Restore. Die Nacht/Tag-Entscheidung läuft jetzt zentral über
  `shouldNightBeActive()` (Boot, WebGUI-Speichern und periodischer
  Übergangs-Check nutzen dieselbe Logik).

### Changed

- **Nachtmodus-Schrift deutlich dunkler.** Der Haupttext der Nachtpalette
  wird nicht mehr um 15 % aufgehellt, sondern um 35 % Richtung Schwarz
  abgedunkelt (Basiston bleibt Crimson rgb(176,0,32)); Sekundärtext
  (`COL_DIM`) geht von 45 % auf 60 % Richtung Schwarz, damit Haupt- und
  Zweittext unterscheidbar bleiben. Zusammen mit dem gedimmten Backlight
  ergibt das ein erheblich dunkleres Nachtbild.

## [1.0.7] — Stable

### Added

- **WebGUI-Login zur Laufzeit änderbar / veröffentlichbares Firmware-Binary.**
  Benutzername und Passwort der WebGUI liegen jetzt in der Laufzeit-Config
  (NVS) und sind im Einstellungen-Tab unter „Netzwerk → WebGUI-Zugang"
  änderbar (Passwort write-only wie beim MQTT-Passwort: leer = behalten,
  Checkbox = entfernen; leeres Passwort deaktiviert die Anmeldung). Die
  compile-time-Werte aus `Config.h`/`Config.local.h` sind nur noch
  First-Boot-Seeds. Neues PlatformIO-Env `esp32_2432s028r_public`
  (`-D INFOTERM_PUBLIC_BUILD`): ignoriert `Config.local.h` vollständig —
  keine WLAN-Seeds (Erstinbetriebnahme über das SoftAP-Setup-Portal) und der
  dokumentierte Standard-Login `admin`/`infoterm`. `tools/make_release.py`
  baut daraus zusätzlich `InfoTerm_x_y_z_public.bin`; nur diese Datei darf an
  ein GitHub-Release angehängt werden (die private `_stable`/`_nonstable`-Bin
  enthält weiterhin lokale Seeds und bleibt lokal). Der Backup-Export enthält
  den Benutzernamen, nie das Passwort. Quellcode-Builds ohne `Config.local.h`
  haben jetzt ebenfalls Login `admin`/`infoterm` als Default (vorher offen)
  und keine Platzhalter-SSID mehr.
- **Neustart-Button in der WebGUI.** Auf der Einstellungsseite neben dem
  Speichern-Button; mit Bestätigungsdialog (lokalisiert in allen 8 Sprachen).
  Der `POST /restart`-Endpunkt ist durch Login + CSRF-Token geschützt; der
  eigentliche `ESP.restart()` läuft verzögert in `loop()`, damit die
  HTTP-Antwort den Browser noch erreicht. Die Seite lädt nach ~20 s
  automatisch neu.

### Fixed

- **Setup-Portal: Display flimmerte stark.** Der Portal-Bildschirm setzte
  `lastDrawnPage` nicht, dadurch feuerte der Redraw-Trigger in `loop()`
  (`pageDirty || lastDrawnPage != currentPage`) in jeder Iteration und die
  Portal-Seite wurde ~100× pro Sekunde inkl. `fillScreen` neu gezeichnet.
- **Setup-Portal: AP war vom Handy aus kaum bzw. gar nicht zu finden.** Der
  WiFi-Reconnect-Scheduler startete alle 5 s einen bis zu 15 s laufenden
  STA-Verbindungsversuch; im `WIFI_AP_STA`-Modus zieht das das gemeinsame Radio
  über alle Kanäle und der SoftAP-Beacon wird für Telefone praktisch
  unsichtbar. Dazu kam: Der Arduino-Core wiederholt einen fehlgeschlagenen
  Verbindungsversuch von sich aus endlos (Auto-Reconnect ist per Default an
  und `NO_AP_FOUND` gilt als „reconnectable"), sodass selbst ein einzelner
  `WiFi.begin()` das Radio dauerhaft über die Kanäle zog — das Netz war zwar
  zeitweise sichtbar, aber der Beitritt (Handshake) schlug fehl. Fix: Bei
  aktivem Portal wird der Core-Auto-Reconnect deaktiviert und die STA-Seite
  nach jedem fehlgeschlagenen Zyklus per `WiFi.disconnect()` stillgelegt;
  die periodischen Reconnect-Versuche des Schedulers sind auf einen Versuch
  pro 90 s gedrosselt (`PORTAL_RECONNECT_INTERVAL_MS`). Der Setup-AP bleibt
  damit stabil sichtbar und beitretbar. Das automatische Schließen des
  Portals, wenn ein konfiguriertes Netz zurückkehrt, bleibt erhalten (nur
  verzögert); über das Portal eingegebene Zugangsdaten verbinden weiterhin
  sofort, und beim Schließen des Portals wird Auto-Reconnect wieder
  aktiviert.

## [1.0.6] — Stable

### Added

- **SoftAP-Onboarding-Portal.** Verbindet sich beim Start kein hinterlegtes
  WLAN, öffnet das InfoTerm einen WPA2-geschützten Setup-Access-Point
  (`InfoTerm-Setup-XXXX`) und zeigt AP-Name, Passwort und `http://192.168.4.1`
  auf dem Display. Ein Captive-Portal (DNS-Catch-all + Redirect) öffnet sich am
  Handy automatisch: Netz aus der Scan-Liste wählen, Passwort eingeben,
  verbinden — das Gerät kann so komplett **ohne Config-Dateien** in ein WLAN
  aufgenommen werden. Das Portal schließt sich automatisch, sobald die
  STA-Verbindung steht (Access Point läuft im `WIFI_AP_STA`-Modus, damit
  Verbindungsversuche ihn nicht abreißen). Zusätzlich ein manueller Trigger
  „Setup-Portal starten" (Button im Netzwerk-Block, auth + CSRF). AP-Name und
  -Passwort sind über `INFOTERM_AP_SSID` / `INFOTERM_AP_PASSWORD` in
  `Config.local.h` überschreibbar; das Portal lässt sich per
  `INFOTERM_PORTAL_ENABLED 0` abschalten.

### Fixed

- **JS-Strings mit Apostroph brachen die WebGUI in einigen Sprachen.** Die
  lokalisierten JavaScript-Meldungen wurden als einfach-gequotete Literale
  eingesetzt; Übersetzungen mit Apostroph (z. B. Französisch „L'interface",
  Italienisch „L'AP") hätten das injizierte `T`-Objekt und damit das gesamte
  Inline-Skript zerschossen. Jetzt werden Werte über einen `jsQuote()`-Helfer
  escaped (Backslash + Apostroph).

## [1.0.5] — Stable

### Added

- **WLAN-Auswahl in der WebGUI.** InfoTerm kann jetzt ohne Config-Dateien in ein
  anderes WLAN aufgenommen werden. Drei editierbare WLAN-Slots (SSID + Passwort)
  im Einstellungen-Tab, aus den compile-time-Netzen gesät und in NVS
  gespeichert. Ein „WLAN suchen"-Button (`/wifi/scan`, auth + CSRF) listet die
  sichtbaren Netze; die SSID wird per Vorschlagsliste im SSID-Feld gewählt oder
  eingetippt (auch versteckte Netze). Passwörter sind write-only (werden nie
  angezeigt) und werden **nie** ins Backup exportiert (SSIDs schon). Die
  WLAN-State-Machine nutzt die Runtime-Slots mit geordnetem Fallback (leere
  Slots werden übersprungen); eine Änderung löst einen Reconnect aus. Die
  compile-time-Netze bleiben als Fallback erhalten.

### Changed

- **Einstellungen-Tab neu gegliedert** in die Blöcke **Allgemein**,
  **Netzwerk** (WLAN-Slots + IP zusammengeführt) und **Standort** (Einheiten,
  Datumsformat, Zeitzone dorthin verschoben). Feldbeschreibungen stehen jetzt
  direkt zwischen Titel und Eingabefeld. „IP-Bezug" in **„IP-Zuweisung"**
  umbenannt.

## [1.0.4] — Stable

### Changed

- **Partitionstabelle: größere OTA-App-Slots.** Beide OTA-Slots wurden von
  1,625 MB (`0x1A0000`) auf **1,75 MB (`0x1C0000`)** vergrößert, um
  Flash-Reserve für kommende Funktionen (WLAN-Onboarding) zu schaffen. Die
  zusätzlichen 256 KB stammen vollständig aus dem Tail: `spiffs` 384 KB → 128 KB
  (die Backup/Restore-JSON ist nur ~4 KB), `coredump` bleibt 64 KB. `nvs`
  (256 KB) und `otadata` (auf `0xE000`) bleiben unangetastet, `app0` bleibt bei
  Offset `0x50000` (kein `board_upload.offset_address`-Wechsel nötig). Dual-Slot
  OTA bleibt vollständig erhalten. Flash-Auslastung sinkt von 83,3 % auf 77,3 %.
- **Migration (nur einmalig, nur bei bestehenden Geräten):** Weil sich die
  Offsets von `app1`/`coredump`/`spiffs` verschieben, wird die neue Tabelle erst
  durch einen **USB-Flash mit einmaligem Full-Erase** wirksam
  (`pio run -t erase` dann `pio run -t upload`). Der Erase löscht NVS — vorher
  eine Konfigurations-Sicherung exportieren und danach wieder importieren
  (die WLAN-Zugangsdaten sind compile-time und überleben den Erase). Ein reines
  OTA-Update behält die alte Tabelle bei.

## [1.0.3] — Stable

### Added

- **Mehrsprachigkeit (8 Sprachen)**: Neben Deutsch und Englisch jetzt auch
  **Französisch, Spanisch, Italienisch, Russisch, Hindi und Chinesisch**,
  umschaltbar in den Einstellungen. Die Sprachtabelle (`Language.h`) wurde auf
  eine indizierte Struktur `t[LANG_COUNT]` umgestellt; ein `static_assert`
  sichert die Synchronität von Enum und Tabelle.
- **Display-Fallback für nicht-lateinische Schriften**: Russisch, Hindi und
  Chinesisch werden in der WebGUI voll dargestellt. Da die eingebetteten
  TFT-Fonts keine kyrillischen/indischen/chinesischen Glyphen enthalten, fällt
  das Geräte-Display für diese Sprachen automatisch auf Englisch zurück
  (`TXT()` = display-sicher, `TXTW()` = volle Sprache für die WebGUI).
- **VPN-Name und Tunnel-IP auf der Info-Seite**: Die Status-/Info-Seite des
  Terminals zeigt zusätzlich zum VPN-Status jetzt den VPN-Namen und die
  Tunnel-Adresse als eigene Zeilen.

### Fixed

- **Durchgängige Übersetzung der WebGUI**: Zahlreiche fest verdrahtete deutsche
  Texte (Labels, Beschreibungen, Dropdown-Optionen, JavaScript-Meldungen,
  Farbnamen, Widget-Positionen wie „Oben links", MQTT-Testmeldungen) liefen
  bisher nicht über das Sprachsystem und blieben auch bei anderer Sprachwahl
  deutsch — jetzt vollständig übersetzt.
- **Datei-Auswahl-Buttons lokalisiert**: Der browser-native „Datei
  auswählen"-Text der Upload-Felder (Backup/Firmware/VPN/MQTT-Import) wurde
  durch einen eigenen, gestylten Button ersetzt, der der gewählten UI-Sprache
  folgt statt der Browsersprache.
- **Sprachauswahl wurde für neue Sprachen zurückgesetzt**: Eine Whitelist im
  Speichern-Handler ließ nur `de`/`en` zu und setzte alle anderen Sprachen
  still auf Deutsch zurück. Jetzt werden alle acht Sprachcodes akzeptiert.

## [1.0.2] — Stable

### Added

- **Tab-Rotation (Einbrennschutz)**: Neue Option im Einstellungen-Tab
  direkt unter dem Schlafmodus (Checkbox + Dropdown 30 / 60 / 180 / 360
  Sekunden). Wenn der automatische Schlafmodus auf "Nie" steht, laufen die
  angezeigten Tabs (Home und aktive Custom-Tabs, **ausser dem Info-Tab**)
  automatisch im gewaehlten Intervall durch, damit kein statisches Bild
  einbrennt. Umgesetzt ueber `handleTabCycle()` (aufgerufen im
  5-Sekunden-`schedulerSystemStatus`-Task, greift nur bei
  `sleepIntervalMin == 0`); manuelle Touch-Bedienung und ein WebGUI-Save
  setzen den Rotations-Timer zurueck, sodass der gerade gewaehlte Tab die
  volle Intervalldauer stehen bleibt. Persistiert in NVS und Teil von
  Backup/Restore.

## [1.0.1] — Stable

### Fixed

- **Bildschirm-Flackern der Info-Seite behoben (Hauptursache)**: Der
  1-Hz-Refresh der Info-/Status-Seite (`updateStatusDynamic()`) loeschte
  bei jeder Aenderung den kompletten Statusbereich per `fillRect` und
  zeichnete alle neun Zeilen von oben nach unten neu. Die Vergleichs-
  Zeichenkette enthielt das WLAN-Signal in dBm, das staendig schwankt -
  dadurch baute sich die Info-Seite unregelmaessig alle paar Sekunden
  komplett neu auf. Jetzt wird pro Zeile ein eigener Cache gefuehrt und
  nur die tatsaechlich geaenderte Zeile neu gezeichnet (`drawSetupRow()`
  loescht ohnehin nur ihre eigene Zeile); der Vollbereich-`fillRect`
  entfaellt.
- **Weiteres Flackern auf den Widget-Seiten behoben**: Mehrere Ereignisse
  loesten ueber ein faelschliches `pageDirty` einen kompletten
  Vollbild-Neuaufbau aus, obwohl es sich nur um Wert-/Statusaenderungen
  handelte:
  - jedes eingehende MQTT-Wetter-Update (`markMqttWeatherUpdated()`,
    WeatherFlow-Observation alle paar Sekunden) - haeufigstes Flackern;
  - jeder WLAN-Reconnect (Erfolg wie Timeout) - auch auf dem Home-Screen,
    der gar keinen Verbindungsstatus anzeigt;
  - jeder MQTT-(Re)connect.
  WLAN-/MQTT-Reconnect setzen jetzt nur noch `dataDirty`; der
  Verbindungsstatus wird ueber `updateStatusDynamic()` (Teilbereich-Redraw
  mit Change-Detection) gezeigt. Das Wetter-Update invalidiert stattdessen
  die Widget-Caches (`clearHomeSlotCaches()` plus Custom-Tab-Caches, genau
  wie der bereits vorhandene MQTT-Datenpunkt-Pfad), sodass der
  1-Hz-Refresh die Wetter-Widgets per Sprite neu zeichnet - die Werte
  aktualisieren weiterhin, nur ohne Vollbild-Neuaufbau. Ursache jeweils als
  Code-Kommentar dokumentiert.

## [1.0.0] — Stable

