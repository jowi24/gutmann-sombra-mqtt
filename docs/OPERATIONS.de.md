🇬🇧 [English](OPERATIONS.md) · 🇩🇪 **Deutsch**

# Betrieb, Diagnose und Flashen

Dieses Dokument fasst die am 16. August 2026 ermittelten Hardwaredaten, das
beobachtete Fehlerbild und die Bedienung der Firmware ab Version 0.0.7
zusammen.

## Hardware

| Eigenschaft | Wert |
| --- | --- |
| Plattform | ESP8266 |
| Erkannter Chip | ESP8266EX |
| Fähigkeiten | WLAN, bis 160 MHz |
| Quarz | 26 MHz |
| MAC-Adresse | `68:c6:3a:88:b4:ba` |
| PlatformIO-Board | `nodemcuv2` |
| Schaltungsentwurf | ESP8266-12E-Modul |
| Serieller Adapter | `/dev/cu.usbserial-AD025JRF` bzw. `/dev/tty.usbserial-AD025JRF` |
| Firmware-Baudrate | 115200 Baud |
| ROM-Bootausgabe | 74880 Baud |

Die MAC-Adresse identifiziert dieses konkrete Gerät eindeutig. Die während der
Diagnose verwendete Adresse `192.168.178.48` wurde dynamisch per DHCP vergeben
und kann sich ändern.

## Ursprüngliches Fehlerbild

- Das Gerät wird regelmäßig ein- bis zweimal täglich von der
  Stromversorgung getrennt.
- Dieses Verhalten funktionierte mehrere Wochen.
- Anschließend blieb das Gerät etwa fünf Tage dauerhaft in MQTT offline.
- Im Pi-hole erschien nach einem Neustart keine DNS-Anfrage für
  den MQTT-Broker.
- Die bekannten DHCP-Einträge gehörten nicht zum Gerät; dessen MAC-Adresse war
  dort nicht vorhanden.
- Nach dem Ausbau und einem Start an einem Standort mit stärkerem WLAN
  verband sich das Gerät wieder.

Der Test außerhalb des Einbauorts beweist die grundsätzliche Funktion von
ESP8266, Flash, Firmwarekonfiguration, WLAN und MQTT. Er bewertet nicht die
Funkqualität am Einbauort.

## Wahrscheinliche Ursache

Die alte Firmware verwendete einen blockierenden Aufruf von
`WiFiManager::autoConnect()`. Schlug die erste WLAN-Verbindung beim Einschalten
wegen des schwachen Signals fehl, wechselte das Gerät in ein dauerhaft
blockierendes Konfigurationsportal. Die spätere Reconnect-Logik in `loop()`
wurde dann nicht erreicht.

Das fehlende Setup-WLAN am Einbauort schließt diesen Zustand nicht aus: Das
Signal des ESP8266 kann durch denselben Einbauort ebenfalls stark gedämpft
werden.

Zusätzlich bleibt die eingebaute Stromversorgung eine mögliche Fehlerquelle.
Ein erfolgreicher Betrieb über USB kann eine schwache oder instabile
3,3-V-Versorgung verdecken.

## Verhalten ab Firmware 0.0.7

1. Die Firmware startet die Haubensteuerung ohne auf das WLAN zu warten.
2. Sie versucht alle zehn Sekunden, das gespeicherte WLAN zu erreichen.
3. Nach 20 Sekunden ohne Verbindung startet zusätzlich der geschützte Hotspot
   `HoodControl-Setup`.
4. Das Portal ist unter `http://192.168.4.1` erreichbar.
5. Während der Hotspot aktiv ist, versucht das Gerät alle 30 Sekunden erneut,
   das gespeicherte WLAN zu erreichen.
6. Nach einer Minute stabiler WLAN-Verbindung wird der Hotspot beendet.
7. MQTT-Verbindungsversuche verwenden exponentielles Backoff von fünf Sekunden
   bis maximal fünf Minuten.
8. Nach einem MQTT-Reconnect werden Discovery und alle Haubenzustände erneut
   publiziert.

## Konfigurations-Hotspot

| Eigenschaft | Wert |
| --- | --- |
| SSID | `HoodControl-Setup` |
| Passwort | `hoodcontrol` (Standard, siehe unten) |
| Portal | `http://192.168.4.1` |

Das Passwort muss mindestens acht Zeichen lang sein, da der ESP8266-Hotspot
sonst nicht als geschütztes WLAN gestartet werden kann.

Das Standardpasswort ist öffentlich. Ein eigenes Passwort und den
Standard-MQTT-Broker (der Broker lässt sich auch später im Portal ändern)
beim Bauen setzen, z. B. in `platformio.ini`:

```ini
build_flags =
  -D PORTAL_PASSWORD='"mein-geheimes-pw"'
  -D MQTT_HOST='"mqtt.example.lan"'
```

Nach dem Speichern neuer WLAN- oder MQTT-Einstellungen startet das Gerät
kontrolliert neu.

## MQTT

Der Standardpräfix lautet `home/kitchen/hood`.

### Verfügbarkeit

| Topic | Bedeutung |
| --- | --- |
| `$online` | Retained-Verfügbarkeit, `true` oder Last-Will `false` |
| `$firmware` | Firmwareversion |
| `$ip` | Aktuelle DHCP-Adresse |
| `$wifi_rssi` | WLAN-Signalstärke in dBm |
| `$uptime` | Laufzeit in Sekunden |
| `$reset_reason` | Resetursache des ESP8266 |
| `$last_error` | Zuletzt erkannter Fehler |

### Erweiterte Diagnose

Das retained Topic `diagnostics` enthält unter anderem:

```json
{
  "firmware": "0.0.7",
  "uptime_s": 1234,
  "rssi": -78,
  "wifi_status": 3,
  "ip": "192.168.178.48",
  "mqtt_state": 0,
  "wifi_reconnects": 2,
  "mqtt_reconnects": 3,
  "mqtt_failures": 1,
  "free_heap": 35892,
  "reset_reason": "External System",
  "previous_boot_stage": "running",
  "last_error": "none",
  "portal_active": false
}
```

Das nicht-retained Topic `events` liefert bis zu 16 gepufferte Ereignisse nach,
sobald MQTT wieder erreichbar ist. Beispiele sind WLAN-Abbrüche,
Reconnect-Versuche, Portalstart und MQTT-Fehler.

### RSSI einordnen

| RSSI | Einschätzung |
| --- | --- |
| besser als -60 dBm | gut |
| -60 bis -70 dBm | brauchbar |
| -70 bis -80 dBm | schwach |
| schlechter als -80 dBm | sehr instabil |

Entscheidend ist der Messwert am tatsächlichen Einbauort bei geschlossener
Haube.

## USB-Flashvorgang

### Normaler PlatformIO-Upload

```sh
pio run -e nodemcuv2 -t upload
```

Der automatische Wechsel in den Bootloader funktionierte mit dem vorhandenen
seriellen Adapter nicht zuverlässig und endete mit:

```text
Failed to connect to ESP8266: Timed out waiting for packet header
```

### Erfolgreiches manuelles Verfahren

1. Die einzelne Taste am Modul gedrückt halten.
2. Den seriellen Adapter verbinden beziehungsweise einen Upload mit
   automatischem Reset starten.
3. Die Taste während des Verbindungsaufbaus gedrückt halten.
4. Nach Beginn des Schreibvorgangs kann die Taste losgelassen werden.
5. Nach dem Flashen den Adapter ohne gedrückte Taste neu verbinden, damit die
   Firmware normal startet.

Verwendeter direkter Upload:

```sh
esptool --chip esp8266 \
  --port /dev/cu.usbserial-AD025JRF \
  --baud 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash 0x0 .pio/build/nodemcuv2/firmware.bin
```

Der erfolgreiche Vorgang erkannte den ESP8266EX mit der MAC-Adresse
`68:c6:3a:88:b4:ba`, schrieb 437.760 Bytes und bestätigte anschließend den
Flash-Hash.

Das Drücken der Taste beim Einschalten zieht sehr wahrscheinlich GPIO0 auf LOW
und aktiviert damit den ROM-Bootloader. Ohne gedrückte Taste startet die
Anwendungsfirmware.

(Auf der Platine Rev. 0.2/0.3 wählt stattdessen der Jumper J5 zwischen RUN
und FLASH – siehe [CIRCUIT.de.md](CIRCUIT.de.md), Abschnitt 10.)

## Diagnoseablauf bei erneutem Ausfall

1. Prüfen, ob `home/kitchen/hood/$online` auf `false` gewechselt ist.
2. Nach `HoodControl-Setup` suchen und gegebenenfalls `192.168.4.1` öffnen.
3. In Pi-hole oder dem DHCP-Server nach der MAC-Adresse
   `68:c6:3a:88:b4:ba` suchen.
4. MQTT-Topics `$wifi_rssi`, `$last_error`, `$reset_reason`, `diagnostics` und
   `events` prüfen.
5. Bei vorhandenem USB-Zugang den seriellen Monitor mit 115200 Baud öffnen.
6. Wenn der USB-Seriell-Port sichtbar ist, aber auch der ESP-ROM-Bootloader
   keinerlei Daten sendet, Versorgung, Reset- und Boot-Pins prüfen.

## Elektrische Prüfung

Falls das Gerät nur über USB, aber nicht in der Haube zuverlässig arbeitet,
gegen GND messen:

| Pin | Erwarteter Pegel |
| --- | --- |
| VCC | etwa 3,3 V |
| EN/CH_PD | HIGH, etwa 3,3 V |
| RST | HIGH, etwa 3,3 V |
| GPIO0 | HIGH beim normalen Start |
| GPIO2 | HIGH beim normalen Start |
| GPIO15 | LOW beim normalen Start |

Spannungseinbrüche während WLAN-Sendeimpulsen sind ebenfalls relevant. Die
Firmware verwendet maximale WLAN-Sendeleistung und deaktiviert den
WLAN-Schlafmodus; dadurch ist eine stabile Stromversorgung besonders wichtig.
