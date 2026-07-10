# InfoTerm Release Notes

## Aktueller Stand

Aktuelle stabile Basis: **1.0.5**  
Diese Version: **1.0.5** (stable)

1.0.5 WLAN-Auswahl in der WebGUI: drei editierbare WLAN-Slots (SSID + Passwort),
"WLAN suchen"-Scan der sichtbaren Netze, Auswahl im SSID-Feld - InfoTerm kann so
ohne Config-Dateien in ein anderes WLAN. Passwoerter write-only, nie im Backup.
Ausserdem: Einstellungen-Tab neu gegliedert (Allgemein / Netzwerk / Standort),
"IP-Bezug" -> "IP-Zuweisung". Details siehe `CHANGELOG.md`.

1.0.4 Partitionstabelle: OTA-App-Slots von 1,625 MB auf 1,75 MB vergroessert
(Flash-Reserve fuer kommende Funktionen; Auslastung 83,3 % -> 77,3 %). spiffs
384 KB -> 128 KB, nvs/otadata/OTA unveraendert. Bei bestehenden Geraeten wird
die neue Tabelle nur durch einen einmaligen USB-Full-Erase wirksam (vorher
Config-Backup exportieren, danach importieren). Details siehe `CHANGELOG.md`.

1.0.3 Mehrsprachigkeit: 8 Sprachen (DE/EN/FR/ES/IT/RU/HI/ZH), umschaltbar in
den Einstellungen. Russisch/Hindi/Chinesisch werden in der WebGUI voll
dargestellt; das Geraet-Display faellt fuer diese nicht-lateinischen Schriften
auf Englisch zurueck. Ausserdem: durchgaengige Uebersetzung der WebGUI
(inkl. Datei-Buttons und Widget-Positionen) und VPN-Name/Tunnel-IP auf der
Info-Seite. Details siehe `CHANGELOG.md`.

1.0.2 Neue Option "Tab-Rotation (Einbrennschutz)": Bei Schlafmodus "Nie"
laufen die angezeigten Tabs (ausser Info) automatisch im gewaehlten
Intervall (30/60/180/360 s) durch. Details siehe `CHANGELOG.md`.

1.0.1 Bugfix: Bildschirm-Flackern behoben (Info-Seite baute sich wegen des
staendig schwankenden WLAN-Signals alle paar Sekunden komplett neu auf;
jetzt zeilenweise Aktualisierung). Details siehe `CHANGELOG.md`.

1.0.0 Erstes Release
