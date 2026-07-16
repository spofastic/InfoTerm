# InfoTerm Release Notes

## Aktueller Stand

Aktuelle stabile Basis: **1.0.11**  
Diese Version: **1.0.11** (stable, vollstaendig hardware-verifiziert)

1.0.11 Design-&-Tabs-Export (Issue #8) + UI-Feinschliff: Neuer Block auf
dem Design-Tab exportiert/importiert NUR die Design-Einstellungen
(Farben, Nachtmodus, Widget-Rahmen/-Schriftgroesse, Rotation) und die
Custom-Tab-Konfiguration als JSON — eigener Dateityp-Marker, Voll-Backups
und DataPoints-Dateien werden abgelehnt; DataPoints, Feeds und
Netzwerk/System bleiben aussen vor. Ausserdem sitzt der "Aktiv"-Haken
der Custom-Tabs jetzt oben neben der Tab-Ueberschrift. Details siehe
`CHANGELOG.md`.

1.0.10 RSS-Feeds (Issue #7): Neuer WebGUI-Tab "RSS Feed" (zwischen
DataPoints und Design) verwaltet bis zu 4 Feed-URLs. Jeder Custom-Tab
kann per neuem "Inhalt"-Dropdown statt seiner Widgets den neuesten
Eintrag eines gewaehlten Feeds anzeigen; zu lange Eintraege scrollen
langsam vertikal ueber das Display. Abruf alle 10 Minuten (RSS 2.0 +
Atom, HTTP/HTTPS), Inhalte werden fuer die Display-Fonts nach ASCII
transliteriert. Feed-Liste und Tab-Auswahl persistiert + im Backup.
Details siehe `CHANGELOG.md`.

1.0.9 Widget-Schriftgroesse + freeHeap-DataPoint: Neues Dropdown
"Widget-Schriftgroesse" (Klein/Normal/Gross) im Design-Tab - "Klein"
zeichnet die Widget-Werte in 16 px, "Normal" wie bisher in 26 px, "Gross"
in 32 px; fuer "Gross" zu breite Werte fallen automatisch pro Widget auf
Normal zurueck statt abgeschnitten zu werden. Layout und Widget-Anzahl
bleiben unveraendert. Ausserdem neuer
System-Datenpunkt `system.freeHeap` (freier Heap in KB) fuer den
DataPoints-Tab und als Widget. Details siehe `CHANGELOG.md`.

1.0.8 Nachtmodus-Ausbau: neues Dropdown "Nacht-Helligkeit" im Design-Tab
(10/25/50/75/100 %, Standard 25 %) - solange die Nachtpalette aktiv ist,
wird das Display-Backlight per PWM entsprechend gedimmt; die Sleep-Stufen
(Dimmen/Aus) greifen unveraendert zusaetzlich. Ausserdem wird die Schrift
im Nachtmodus deutlich dunkler dargestellt (Haupttext 35 % Richtung
Schwarz statt 15 % aufgehellt, Sekundaertext 60 % statt 45 %). Neuer
Schalter "Nachtmodus erzwingen" aktiviert die Nachtpalette samt Dimmung
sofort auch tagsueber, unabhaengig vom Sonnenfenster (z. B. zum Testen
oder fuer dauerhaft dunklen Betrieb). Alle Werte sind persistiert und
Teil von Backup/Restore. Details siehe `CHANGELOG.md`.

1.0.7 Setup-Portal-Fixes, Neustart-Button und oeffentliches Firmware-Binary:
Das Setup-Portal ist jetzt praktisch nutzbar - das Display flimmert nicht
mehr (Redraw-Schleife behoben) und der Setup-AP bleibt vom Handy aus
beitretbar (Core-Auto-Reconnect wird bei aktivem Portal stillgelegt,
Reconnect-Versuche auf einen pro 90 s gedrosselt). Neuer Neustart-Button
neben "Speichern" (Login + CSRF, Bestaetigungsdialog). WebGUI-Login
(Benutzername/Passwort) liegt jetzt in der Laufzeit-Config und ist unter
"Netzwerk -> WebGUI-Zugang" aenderbar; die Compile-Zeit-Werte sind nur noch
First-Boot-Seeds. Neues Build-Env `esp32_2432s028r_public` erzeugt ein
Binary ohne lokale Secrets (`InfoTerm_x_y_z_public.bin`) - erstmals liegt
damit eine fertige Firmware beim GitHub-Release bei (Erstinbetriebnahme
ueber das Setup-Portal, Login admin/infoterm, danach aendern). Details
siehe `CHANGELOG.md`.

1.0.6 SoftAP-Onboarding-Portal: verbindet sich beim Start kein hinterlegtes
WLAN, oeffnet das Geraet einen WPA2-Setup-Access-Point mit Captive-Portal
(SSID/Passwort/URL auf dem Display) - so laesst sich das InfoTerm ohne
Config-Dateien in ein WLAN aufnehmen. Auto-Exit sobald verbunden; manueller
Trigger im Netzwerk-Block. Ausserdem Bugfix: JS-Strings mit Apostroph
(FR/IT) brachen die WebGUI - jetzt via jsQuote escaped. Details siehe
`CHANGELOG.md`.

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
