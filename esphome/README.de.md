🇬🇧 [English](README.md) · 🇩🇪 **Deutsch**

# ESPHome-Firmware

Alternative zur PlatformIO-Firmware in [`src/`](../src/main.cpp) für
dieselbe Hardware (Platine Rev. 0.2/0.3). Statt per MQTT-Discovery erscheint
die Haube über die native ESPHome-API in Home Assistant.

> **Stand:** kompiliert mit ESPHome 2026.11, an der Haube noch nicht getestet.

## Funktionsweise

Der zeitkritische Teil lässt sich nicht in YAML abbilden: Die Haube fragt
ihre Tastenmatrix im 1-ms-Raster ab (siehe [CIRCUIT.de.md](../docs/CIRCUIT.de.md),
Abschnitt 3), und die Hauptschleife von ESPHome ist dafür viel zu langsam und
ungleichmäßig. Die External Component [`sombra_hood`](components/sombra_hood)
enthält deshalb die ISR-Logik der Arduino-Firmware nahezu unverändert:

- Ein Interrupt auf der SCAN-Leitung (GPIO13) synchronisiert sich etwa alle
  50 ms neu auf den Beginn einer 4-ms-Runde.
- Der Hardware-Timer `timer1` feuert alle 100 µs und liest im passenden
  Fenster die LED-Spalten bzw. legt die Tastenspalten auf HIGH.
- `loop()` entprellt die LED-Zustände (80 ms, beim Ausgehen der blinkenden
  Filteranzeige 1,5 s) und gibt sie an die Entities weiter.

Alles andere (WLAN mit Setup-Hotspot, OTA, Status-LED, Diagnose) erledigt
ESPHome selbst.

## Entities

| Plattform | Standardname | Funktion |
|---|---|---|
| `fan` | Lüftung | Stufen 1–4 (25/50/75/100 %), aus |
| `light` | Haubenlicht | Licht |
| `switch` | Nachlauf-Timer | Nachlauf-Timer |
| `binary_sensor` | Filterreinigung | Filter-Reinigungsanzeige (`problem`) |
| `button` | Filteranzeige zurücksetzen | Setzt die Anzeige zurück (6 s Tastendruck, nur wenn sie aktiv ist) |

Die Zustände kommen immer von den LEDs des Bedienfelds. Befehle drücken nur
die passende Taste; der Zustand ändert sich, sobald die Haube umgeschaltet
hat. Alle 2 s werden die Entities mit den LEDs abgeglichen, damit nach einem
verlorenen Tastendruck kein falscher Zustand stehen bleibt.

## Bauen und flashen

```sh
cd esphome
cp secrets.yaml.example secrets.yaml   # ausfüllen
esphome run hood-control.yaml
```

Der erste Wechsel von der Arduino-Firmware geht per OTA: mit
`esphome compile hood-control.yaml` bauen und
`.esphome/build/hood-control/.pioenvs/hood-control/firmware.bin` mit
`espota` (dem PlatformIO-OTA) hochladen, oder seriell über J2 mit J5 auf
FLASH, siehe [OPERATIONS.de.md](../docs/OPERATIONS.de.md).

Ohne lokale Kopie lässt sich die Komponente auch direkt von GitHub holen:

```yaml
external_components:
  - source: github://jowi24/gutmann-sombra-mqtt@master
    components: [sombra_hood]
```

## Hinweise

- `timer1` gehört exklusiv der Komponente. Nicht mit Komponenten
  kombinieren, die auf dem ESP8266 ebenfalls `timer1` nutzen (z. B.
  `esp8266_pwm`, `ac_dimmer`, `opentherm`).
- Während eines OTA-Updates werden Timer und SCAN-Interrupt automatisch
  pausiert, wie in der Arduino-Firmware.
- Die Entity-IDs in Home Assistant unterscheiden sich von der MQTT-Version.
  Automationen und Dashboards müssen nach dem Umstieg angepasst werden. Wer
  weiter MQTT möchte, ergänzt die Komponente `mqtt:` in der YAML.
- Nicht portiert: Ereignispuffer (`events`), Boot-Stage und
  MQTT-Fehlerzähler aus der Diagnose der Arduino-Firmware. Das ESPHome-Log
  (`esphome logs`) und die Komponente `debug` übernehmen diese Rolle.
