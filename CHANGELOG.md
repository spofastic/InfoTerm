# Changelog

Alle nennenswerten Änderungen an InfoTerm werden in dieser Datei dokumentiert.

Das Projekt orientiert sich am Format „Keep a Changelog“.

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

