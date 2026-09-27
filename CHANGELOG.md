# Changelog

Alle wesentlichen Änderungen an der Firmware werden in diesem Dokument
festgehalten.

## 0.0.9 - 2026-09-26

Anpassung an die gefertigte Platine (Rev. 0.3), siehe `docs/SCHALTUNG.md`.

### Geändert

- Die Tasten-Ausgänge GPIO12/14 werden jetzt immer aktiv getrieben:
  HIGH = Taste drücken, LOW = loslassen. Früher wurden sie zum Loslassen
  hochohmig geschaltet; das lässt die Eingänge des Pegelwandlers U3 floaten
  und erzeugt Phantom-Tastendrücke.
- GPIO12/14 werden gleich zu Beginn von `setup()` auf LOW gesetzt.
- LED-Pins (`ledPin1/2`) und Tasten-Pins (`buttonPin1/2`) getauscht – beide
  Kanalpaare kommen auf der Platine gegenüber der Lochraster-Version
  vertauscht an.

### Behoben

- Home-Assistant-Discovery der Lüftung: `state_value_template` lieferte
  `on`/`off`, Home Assistant vergleicht das Ergebnis aber mit
  `payload_on`/`payload_off` (`1`/`0`). Die Lüftung stand deshalb dauerhaft
  auf `unknown`. Das Template liefert jetzt `1`/`0`.
- Lüftungsstufe `0` wird als Preset `None` gemeldet und setzt das Preset
  zurück, statt als ungültiges Preset verworfen zu werden.

## 0.0.8 - 2026-09-08

### Hinzugefügt

- Status-LED auf GPIO2 (D4, blaue Onboard-LED des ESP-12, aktiv LOW).
  Das Blinkmuster zeigt die Boot- und Verbindungsphase ohne serielle
  Konsole an: drei kurze Blitze beim Einschalten, schnelles Blinken
  während der WLAN-Anmeldung, mittleres Blinken beim MQTT-Verbinden,
  langsames Blinken bei geöffnetem Konfigurationsportal und ein kurzer
  Heartbeat alle drei Sekunden, sobald beides steht.

## 0.0.7 - 2026-08-16

### Hinzugefügt

- Nicht blockierender WLAN-Start mit paralleler Haubensteuerung.
- Geschützter Fallback-Hotspot `HoodControl-Setup`.
- Konfigurationsportal unter `http://192.168.4.1`.
- Regelmäßige WLAN-Wiederverbindungsversuche im AP+STA-Betrieb.
- Eindeutige MQTT-Client-ID aus Gerätename und ESP8266-Chip-ID.
- MQTT-Diagnose unter `diagnostics`, `$wifi_rssi`, `$uptime`,
  `$reset_reason`, `$firmware`, `$ip` und `$last_error`.
- Gepufferte Verbindungsereignisse unter dem MQTT-Topic `events`.
- Persistente Bootphase zur Diagnose unvollständiger Startvorgänge.
- WLAN-, MQTT- und Fehlerzähler.
- Vollständige Neuübertragung aller Haubenzustände nach MQTT-Reconnect.

### Geändert

- MQTT-Wiederverbindungen verwenden exponentielles Backoff bis fünf Minuten.
- MQTT-Sockettimeout wurde auf drei Sekunden reduziert.
- MQTT-Befehle werden mit QoS 1 abonniert.
- Der Fallback-Hotspot wird nach einer Minute stabiler WLAN-Verbindung
  abgeschaltet.
- Konfigurationsänderungen führen nach erfolgreichem Speichern zu einem
  kontrollierten Neustart.
- Fehler beim Lesen oder Schreiben der Konfiguration sowie beim Publizieren
  und Abonnieren werden sichtbar protokolliert.

### Behoben

- Ein fehlgeschlagener WLAN-Start kann die Firmware nicht mehr dauerhaft im
  blockierenden WiFiManager-Portal festsetzen.
- Zustandsänderungen während einer MQTT-Unterbrechung werden nach dem
  Wiederverbinden erneut synchronisiert.
- Eine feste MQTT-Client-ID kann nicht mehr mit einer zweiten gleichnamigen
  Installation kollidieren.

### Validierung

- Erfolgreicher Build für `nodemcuv2` und `nodemcuv2-ota`.
- RAM: 35.892 von 81.920 Bytes (43,8 %).
- Flash: 433.608 von 1.044.464 Bytes (41,5 %).
- Firmware erfolgreich per USB geschrieben und anhand des Flash-Hashes
  verifiziert.

## 0.0.6 - 2026-07-14

- WLAN-Auto-Reconnect aktiviert.
- Maximale WLAN-Sendeleistung eingestellt.
- WLAN-Schlafmodus deaktiviert.
- Aktive WLAN-Prüfung und MQTT-Verbindungsversuche nur bei bestehender
  WLAN-Verbindung ergänzt.

## 0.0.5

- Firmware zu PlatformIO migriert.
- Homie durch WiFiManager und PubSubClient ersetzt.
- Home-Assistant-MQTT-Discovery ergänzt.
- Lüfter-Presets, Licht-Entity und OTA-Unterstützung ergänzt.
