🇬🇧 **English** · 🇩🇪 [Deutsch](README.de.md)

# ESPHome firmware

Alternative to the PlatformIO firmware in [`src/`](../src/main.cpp) for the
same hardware (PCB rev. 0.2/0.3). Instead of MQTT discovery, the hood shows
up in Home Assistant through the ESPHome native API.

> **Status:** compiles with ESPHome 2026.11, not yet tested on the hood.

## How it works

The time-critical part cannot be built from YAML: the hood scans its key
matrix in 1 ms steps (see [CIRCUIT.md](../docs/CIRCUIT.md), section 3), and
ESPHome's main loop is far too slow and jittery for that. The external
component [`sombra_hood`](components/sombra_hood) therefore contains the ISR
logic of the Arduino firmware almost unchanged:

- An interrupt on the SCAN line (GPIO13) resynchronises roughly every 50 ms
  on the start of a 4 ms scan round.
- The hardware timer `timer1` fires every 100 µs and, in the right
  window, reads the LED columns or drives the key columns HIGH.
- `loop()` debounces the LED states (80 ms, 1.5 s when the blinking
  filter indicator goes off) and passes them on to the entities.

Everything else (Wi-Fi with setup hotspot, OTA, status LED, diagnostics) is
handled by ESPHome itself.

## Entities

| Platform | Default name | Function |
|---|---|---|
| `fan` | Lüftung | Speeds 1–4 (25/50/75/100 %), off |
| `light` | Haubenlicht | Light |
| `switch` | Nachlauf-Timer | Run-on timer |
| `binary_sensor` | Filterreinigung | Filter cleaning indicator (`problem`) |
| `button` | Filteranzeige zurücksetzen | Resets the indicator (6 s key press, only while it is on) |

The states always come from the front-panel LEDs. Commands only press the
corresponding key; the state changes once the hood has switched. Every 2 s
the entities are reconciled with the LEDs, so a command that got lost does
not leave a wrong state behind.

## Building and flashing

```sh
cd esphome
cp secrets.yaml.example secrets.yaml   # fill in
esphome run hood-control.yaml
```

The first flash from the Arduino firmware can be done over the air: build
with `esphome compile hood-control.yaml` and upload
`.esphome/build/hood-control/.pioenvs/hood-control/firmware.bin` with
`espota` (the PlatformIO OTA), or flash serially via J2 with J5 set to
FLASH, see [OPERATIONS.md](../docs/OPERATIONS.md).

Without a local checkout, the component can also be pulled from GitHub:

```yaml
external_components:
  - source: github://jowi24/gutmann-sombra-mqtt@master
    components: [sombra_hood]
```

## Notes

- `timer1` is used exclusively by the component. Do not combine it with
  components that also use `timer1` on the ESP8266 (e.g. `esp8266_pwm`,
  `ac_dimmer`, `opentherm`).
- During an OTA update the timer and the SCAN interrupt are paused
  automatically, as in the Arduino firmware.
- The entity ids in Home Assistant differ from the MQTT version.
  Automations and dashboards need to be adapted after switching. If MQTT
  is still wanted, add the `mqtt:` component to the YAML.
- Not ported: event buffer (`events`), boot stage and MQTT error counters
  from the Arduino firmware's diagnostics. ESPHome's log (`esphome logs`)
  and the `debug` component take over this role.
