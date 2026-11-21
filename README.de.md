🇬🇧 [English](README.md) · 🇩🇪 **Deutsch**

# gutmann-sombra-mqtt

> [!WARNING]
> **Veraltet – wird nicht mehr gepflegt.** Diese Arduino/MQTT-Firmware ist
> durch die ESPHome-Firmware in
> [gutmann-sombra-esphome](https://github.com/jowi24/gutmann-sombra-esphome/blob/main/README.de.md) abgelöst, die auf
> derselben Platine läuft und per OTA eingespielt werden kann. Die Hardware
> ist nach [gutmann-sombra-pcb](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/README.de.md) umgezogen. Dieses
> Repository bleibt für alle verfügbar, die ESPHome nicht verwenden
> möchten, bekommt aber keine neuen Funktionen oder Fehlerbehebungen mehr.

Arduino/PlatformIO-Firmware für ein ESP8266-basiertes IoT-Modul, das eine Dunstabzugshaube (Gutmann Sombra) über
WLAN und MQTT in die Hausautomation einbindet. Die Platine sitzt im Kabel
zwischen Haubenelektronik und Bedienfeld, liest die LEDs des Bedienfelds mit
und kann Tastendrücke nachbilden – das Original-Bedienfeld funktioniert dabei
unverändert weiter.

## Dokumentation

| Dokument | Inhalt |
|---|---|
| [Wie die Schaltung funktioniert](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/docs/CIRCUIT.de.md) | In gutmann-sombra-pcb: Tastenmatrix, Pegelwandler, Stromversorgung, Firmware-Timing |
| [Hardware-Designnotizen](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/kicad/DESIGN.de.md) | In gutmann-sombra-pcb: Bauteilwerte, Layout, Inbetriebnahme-Befunde |
| [Betrieb, Diagnose und Flashen](docs/OPERATIONS.de.md) | Einrichtung, OTA- und serielles Flashen, Diagnose |
| [Änderungsverlauf](CHANGELOG.de.md) | Firmware-Versionen |

Alle Dokumente gibt es auch auf Englisch (`*.md` ohne `.de`).

## Stand

- **Firmware:** 0.0.11 (letzte Version), PlatformIO, Quelltext in
  [`src/main.cpp`](src/main.cpp). Veraltet, siehe Hinweis oben.
- **Hardware:** Platine Rev. 0.2/0.3, siehe [gutmann-sombra-pcb](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/README.de.md).

## Projektstruktur

```
src/main.cpp          Firmware
platformio.ini        Build-Konfiguration (serielles und OTA-Flashen)
docs/                 Betrieb, Diagnose und Flashen
```

## Firmware bauen und flashen

```bash
pio run -e nodemcuv2-ota -t upload
```

Das flasht per WLAN (OTA) auf `hood-control.local`. Für das erste Flashen
oder zur Rettung gibt es `-e nodemcuv2` über einen USB-Seriell-Adapter an J2;
dafür muss der Jumper J5 auf FLASH stehen. Details in
[OPERATIONS.de.md](docs/OPERATIONS.de.md).

## MQTT

Alle Topics liegen unter dem konfigurierten Präfix (Standard
`home/kitchen/hood`). Home Assistant erkennt das Gerät automatisch per
MQTT-Discovery (`homeassistant/…`).

| Topic | Werte | Bedeutung |
|---|---|---|
| `ventilation/state`, `ventilation/set` | `0`–`4` | Lüfterstufe, `0` = aus |
| `light/state`, `light/set` | `true` / `false` | Licht |
| `timer/state`, `timer/set` | `true` / `false` | Nachlauf-Timer |
| `maintenance/state` | `true` / `false` | Filter-Reinigungsanzeige |
| `maintenance/set` | beliebig | setzt die Filteranzeige zurück (6 s Tastendruck, nur wenn sie aktiv ist) |

Diagnose:

- `diagnostics`: zusammengefasster JSON-Status
- `$wifi_rssi`, `$uptime`, `$reset_reason`, `$firmware`, `$ip`, `$last_error`
- `events`: gepufferte Verbindungsereignisse seit dem letzten MQTT-Kontakt
- `$online`: Verfügbarkeit über MQTT Last Will

## Netzwerk

Wenn das gespeicherte WLAN 20 Sekunden lang nicht erreichbar ist, startet die
Firmware den geschützten Hotspot `HoodControl-Setup`. Das Konfigurationsportal
ist unter `http://192.168.4.1` erreichbar. Parallel versucht das Gerät weiterhin,
das gespeicherte WLAN zu erreichen; nach einer stabilen Verbindung wird der
Hotspot wieder beendet.

Die blaue LED auf dem ESP-Modul zeigt den Zustand: schnelles Blinken = WLAN
verbindet, mittleres = MQTT verbindet, langsames = Hotspot aktiv, kurzer
Blitz alle 3 s = alles in Ordnung.

## Hintergrund

Blog-Artikel zur ursprünglichen Lochraster-Version:
[Do-It-Yourself IoT Modul für die Dunstabzugshaube](https://joachim-wilke.de/blog/2017/12/27/do-it-yourself-iot-modul-fur-die-dunstabzugshaube/)

## Lizenz

Copyright (c) 2017–2026 Joachim Wilke.

- **Firmware** (`src/`, `platformio.ini`): [MIT](LICENSE)
- **Dokumentation** (`docs/`, Markdown-Dateien): [CC BY-NC-SA 4.0](LICENSE-DOCS)

Die von der Firmware verwendeten Fremdbibliotheken behalten ihre eigenen Lizenzen.
