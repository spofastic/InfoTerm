# Changelog

Alle nennenswerten Änderungen an InfoTerm werden in dieser Datei dokumentiert.

Das Projekt orientiert sich am Format „Keep a Changelog“.

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

