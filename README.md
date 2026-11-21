🇬🇧 **English** · 🇩🇪 [Deutsch](README.de.md)

# gutmann-sombra-mqtt

> [!WARNING]
> **Deprecated – no longer maintained.** This Arduino/MQTT firmware is
> superseded by the ESPHome firmware in
> [gutmann-sombra-esphome](https://github.com/jowi24/gutmann-sombra-esphome), which runs on the same board and can be
> switched to over the air. The hardware has moved to
> [gutmann-sombra-pcb](https://github.com/jowi24/gutmann-sombra-pcb). This repository stays available for anyone who
> does not want to use ESPHome, but will not receive new features or fixes.

Arduino/PlatformIO firmware for an ESP8266 board that connects a Gutmann Sombra range hood (cooker hood) to
home automation over Wi-Fi and MQTT, with Home Assistant auto-discovery.
The board sits in the cable between the hood electronics and the front
panel. It reads which front-panel LEDs are lit and can simulate key presses.
The original front panel keeps working as before.

- **Hardware:** [gutmann-sombra-pcb](https://github.com/jowi24/gutmann-sombra-pcb)
- **Firmware:** PlatformIO / Arduino, MQTT with Home Assistant discovery,
  OTA updates, Wi-Fi setup portal

## Documentation

| Document | Contents |
|---|---|
| [How the circuit works](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/docs/CIRCUIT.md) | In gutmann-sombra-pcb: key matrix, level shifters, power supply, firmware timing |
| [Hardware design notes](https://github.com/jowi24/gutmann-sombra-pcb/blob/main/kicad/DESIGN.md) | In gutmann-sombra-pcb: part values, layout, bring-up findings |
| [Operation, diagnostics and flashing](docs/OPERATIONS.md) | Setup, OTA and serial flashing, diagnostics |
| [Changelog](CHANGELOG.md) | Firmware versions |

All documents are also available in German (`*.de.md`).

## Status

- **Firmware:** 0.0.11 (final), PlatformIO, source in
  [`src/main.cpp`](src/main.cpp). Deprecated, see the note above.
- **Hardware:** PCB rev. 0.2/0.3, see [gutmann-sombra-pcb](https://github.com/jowi24/gutmann-sombra-pcb).

## Project layout

```
src/main.cpp          firmware
platformio.ini        build configuration (serial and OTA flashing)
docs/                 operation, diagnostics and flashing
```

## Building and flashing the firmware

```bash
pio run -e nodemcuv2-ota -t upload
```

This flashes over Wi-Fi (OTA) to `hood-control.local`. For the first flash
or for recovery, use `-e nodemcuv2` via a USB-serial adapter on J2, with
jumper J5 set to FLASH. Details in [OPERATIONS.md](docs/OPERATIONS.md).

## MQTT

All topics live under the configured prefix (default `home/kitchen/hood`).
Home Assistant picks the device up automatically via MQTT discovery
(`homeassistant/…`).

| Topic | Values | Meaning |
|---|---|---|
| `ventilation/state`, `ventilation/set` | `0`–`4` | fan level, `0` = off |
| `light/state`, `light/set` | `true` / `false` | light |
| `timer/state`, `timer/set` | `true` / `false` | run-on timer |
| `maintenance/state` | `true` / `false` | "clean filter" indicator |
| `maintenance/set` | any | resets the filter indicator (6 s key press, only while it is on) |

Diagnostics:

- `diagnostics`: combined JSON status
- `$wifi_rssi`, `$uptime`, `$reset_reason`, `$firmware`, `$ip`, `$last_error`
- `events`: connection events buffered since the last MQTT contact
- `$online`: availability via MQTT last will

## Network

If the stored Wi-Fi network cannot be reached for 20 seconds, the firmware
starts the password-protected hotspot `HoodControl-Setup`. The setup portal
is at `http://192.168.4.1`. The device keeps trying the stored network in
parallel and shuts the hotspot down once the connection is stable.

The blue LED on the ESP module shows the state: fast blinking = connecting
to Wi-Fi, medium = connecting to MQTT, slow = hotspot active, a short flash
every 3 s = all good.

## Background

Blog post about the original 2017 perfboard version (German):
[Do-It-Yourself IoT Modul für die Dunstabzugshaube](https://joachim-wilke.de/blog/2017/12/27/do-it-yourself-iot-modul-fur-die-dunstabzugshaube/)

## License

Copyright (c) 2017–2026 Joachim Wilke.

- **Firmware** (`src/`, `platformio.ini`): [MIT](LICENSE)
- **Documentation** (`docs/`, Markdown files): [CC BY-NC-SA 4.0](LICENSE-DOCS)

Third-party libraries used by the firmware keep their own licenses.
