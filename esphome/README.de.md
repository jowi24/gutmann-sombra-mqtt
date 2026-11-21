🇬🇧 [English](README.md) · 🇩🇪 **Deutsch**

# ESPHome-Firmware

Empfohlene Firmware für die hood-control-Platine (Rev. 0.2/0.3). Die Haube
erscheint über die native ESPHome-API in Home Assistant. Die
Arduino-Firmware in [`src/`](../src/main.cpp) (MQTT) bleibt als
Rückfallebene erhalten.

> **Stand:** läuft seit 21.11.2026 auf der Haube des Autors (ESPHome
> 2026.11). Getestet: Bedienfeld lesen, alle Tastendrücke, Nachlauf-Timer,
> Neustart ohne Auswirkung auf die Haube, OTA. Noch nicht getestet:
> Filter-Reset.

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

Ohne lokale Kopie lässt sich die Komponente auch direkt von GitHub holen:

```yaml
external_components:
  - source: github://jowi24/gutmann-sombra-mqtt@master
    components: [sombra_hood]
```

## Am eingebauten Modul testen (und zurück)

Beide Firmwares lassen sich in beide Richtungen per OTA tauschen; die Haube
muss dafür nicht geöffnet werden. Beide nutzen dasselbe Flash-Layout (1 MB
Programm, 2 MB LittleFS) und überschreiben deshalb nicht die Einstellungen
der jeweils anderen: WLAN-Zugangsdaten (SDK-Bereich) und MQTT-Konfiguration
der Arduino-Firmware (LittleFS) überstehen den Hin- und Rückweg.

1. Zuerst das Image für den Rückweg bauen, solange sicher ist, dass es dem
   installierten Stand entspricht (Repo-Wurzel, gleiche `build_flags` wie
   bisher):

   ```sh
   pio run -e nodemcuv2-ota
   ```

2. Per Arduino-OTA auf ESPHome wechseln (kein Passwort):

   ```sh
   cd esphome
   esphome compile hood-control.yaml
   python3 ~/.platformio/packages/framework-arduinoespressif8266/tools/espota.py \
     -i hood-control.local -f .esphome/build/hood-control/.pioenvs/hood-control/firmware.bin
   ```

   Home Assistant erkennt das neue ESPHome-Gerät (API-Schlüssel aus
   `secrets.yaml`). Die MQTT-Entities sind währenddessen nicht verfügbar,
   Automationen darauf laufen ins Leere.

3. Testen: `esphome logs hood-control.yaml` zeigt `Synchronised with the hood`
   und jede LED-Änderung. Mit dem Bedienfeld vergleichen, dann Stufe, Licht
   und Timer aus Home Assistant schalten.

4. Per ESPHome-OTA zurück:

   ```sh
   esphome upload hood-control.yaml --device hood-control.local \
     --file ../.pio/build/nodemcuv2-ota/firmware.bin
   ```

   Danach das ESPHome-Gerät in Home Assistant löschen; die MQTT-Entities
   kommen von selbst zurück.

Sicherheitsnetze, falls etwas schiefgeht:

- Nach 10 fehlgeschlagenen Starts in Folge startet ESPHome im *Safe Mode*
  nur mit WLAN und OTA, Schritt 4 funktioniert also weiterhin.
- Ohne WLAN öffnet ESPHome den Hotspot `HoodControl-Setup`; dessen Portal
  unter `http://192.168.4.1` nimmt auch ein Firmware-Upload an.
- Solange die Firmware sie nicht treibt, halten R7/R8 die Tastenausgänge
  auf LOW – eine abstürzende Firmware kann also keine Tasten drücken.
- Letzter Ausweg: seriell über J2 mit J5 auf FLASH flashen, siehe
  [OPERATIONS.de.md](../docs/OPERATIONS.de.md) – dafür muss die Haube
  geöffnet werden.

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
