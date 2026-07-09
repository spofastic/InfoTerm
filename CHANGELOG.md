# Changelog

Alle nennenswerten Änderungen an InfoTerm werden in dieser Datei dokumentiert.

Das Projekt orientiert sich am Format „Keep a Changelog“.

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

