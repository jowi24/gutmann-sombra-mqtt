# homie-hood-control

ESP8266-basiertes IoT-Modul zur Integration einer Dunstabzugshaube in die Hausautomation über MQTT.

- [Änderungsverlauf](CHANGELOG.md)
- [Betrieb, Diagnose und Flashen](docs/OPERATIONS.md)

## Netzwerk und Diagnose

Wenn das gespeicherte WLAN 20 Sekunden lang nicht erreichbar ist, startet die
Firmware den geschützten Hotspot `HoodControl-Setup`. Das Konfigurationsportal
ist unter `http://192.168.4.1` erreichbar. Parallel versucht das Gerät weiterhin,
das gespeicherte WLAN zu erreichen; nach einer stabilen Verbindung wird der
Hotspot wieder beendet.

Unterhalb des konfigurierten MQTT-Präfixes werden Diagnosewerte veröffentlicht:

- `diagnostics`: zusammengefasster JSON-Status
- `$wifi_rssi`, `$uptime`, `$reset_reason`, `$firmware`, `$ip`, `$last_error`
- `events`: gepufferte Verbindungsereignisse seit dem letzten MQTT-Kontakt
- `$online`: Verfügbarkeit über MQTT Last Will

Weitere Informationen im Blog-Artikel:  
[Do-It-Yourself IoT Modul für die Dunstabzugshaube](https://joachim-wilke.de/blog/2017/12/27/do-it-yourself-iot-modul-fur-die-dunstabzugshaube/)
