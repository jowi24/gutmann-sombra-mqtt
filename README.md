🇬🇧 **English** · 🇩🇪 [Deutsch](README.de.md)

# gutmann-sombra-mqtt

An ESP8266 board that connects a Gutmann Sombra range hood (cooker hood) to
home automation over Wi-Fi, either via ESPHome (recommended) or via MQTT
with Home Assistant auto-discovery.
The board sits in the cable between the hood electronics and the front
panel. It reads which front-panel LEDs are lit and can simulate key presses.
The original front panel keeps working as before.

- **Hardware:** KiCad 10 PCB, 80 × 55 mm, through-hole except the ESP-12E
  module, fabrication files included
- **Firmware (recommended):** ESPHome external component, native Home
  Assistant API, OTA updates, Wi-Fi setup hotspot
- **Firmware (fallback):** PlatformIO / Arduino, MQTT with Home Assistant
  discovery, OTA updates, Wi-Fi setup portal

## Documentation

| Document | Contents |
|---|---|
| [How the circuit works](docs/CIRCUIT.md) | Beginner-friendly explanation of the hardware: the hood's key matrix, level shifters, power supply, firmware timing, troubleshooting |
| [Hardware design notes](kicad/DESIGN.md) | Part values, sourcing, layout decisions, bring-up findings |
| [Fabrication files](kicad/fab/FAB.md) | Gerber/drill files and PCB order parameters |
| [Operation, diagnostics and flashing](docs/OPERATIONS.md) | Setup, OTA and serial flashing, diagnostics |
| [ESPHome firmware](esphome/README.md) | Recommended firmware: setup, entities, switching from/to the Arduino firmware |
| [Changelog](CHANGELOG.md) | Firmware versions |

All documents are also available in German (`*.de.md`).

## Status

- **Hardware:** PCB rev. 0.3, see the [schematic](kicad/schematic.pdf) and
  the [layout](kicad/board-layout.pdf). The fabricated rev. 0.2 boards work
  after a small modification at U3 (diodes + pull-downs, see
  [DESIGN.md](kicad/DESIGN.md)).
- **Firmware:** the [ESPHome firmware](esphome/README.md) is the
  recommended variant and runs on the author's hood since 2026-11-21. The
  Arduino firmware 0.0.10 ([`src/main.cpp`](src/main.cpp)) stays available
  for MQTT setups and as a fallback; both can be swapped over the air.

## Project layout

```
esphome/              ESPHome firmware (external component + YAML), recommended
src/main.cpp          Arduino firmware (MQTT), fallback
platformio.ini        build configuration of the Arduino firmware
docs/                 operation and circuit description
kicad/                schematic, PCB, custom footprint and 3D model
kicad/fab/            fabrication files
fritzing/             2017 perfboard version (reference only)
```

## ESPHome firmware (recommended)

```bash
cd esphome
cp secrets.yaml.example secrets.yaml   # fill in
esphome run hood-control.yaml
```

Details, entities and how to switch over from the Arduino firmware via OTA:
[esphome/README.md](esphome/README.md).

## Arduino firmware (MQTT, fallback)

The sections below only apply to the Arduino firmware in `src/`.

### Building and flashing

```bash
pio run -e nodemcuv2-ota -t upload
```

This flashes over Wi-Fi (OTA) to `hood-control.local`. For the first flash
or for recovery, use `-e nodemcuv2` via a USB-serial adapter on J2, with
jumper J5 set to FLASH. Details in [OPERATIONS.md](docs/OPERATIONS.md).

### MQTT

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

### Network

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

Copyright (c) 2017–2026 Joachim Wilke. The firmware is open source (MIT). Hardware
and documentation are free for **non-commercial** use only; commercial use (e.g. selling boards, kits or products based on this project)
requires the author's permission.

- **Firmware** (`src/`, `platformio.ini`, `esphome/` except Markdown files): [MIT](LICENSE)
- **Hardware and documentation** (`kicad/`, `fritzing/`, `docs/`, Markdown files):
  [CC BY-NC-SA 4.0](LICENSE-HARDWARE-DOCS)

Third-party libraries used by the firmware keep their own licenses.
