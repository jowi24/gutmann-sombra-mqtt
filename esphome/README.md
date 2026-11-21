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

Without a local checkout, the component can also be pulled from GitHub:

```yaml
external_components:
  - source: github://jowi24/gutmann-sombra-mqtt@master
    components: [sombra_hood]
```

## Trying it on the installed board (and switching back)

Both firmwares can be swapped over the air in both directions; the hood
does not need to be opened. Both use the same flash layout (1 MB sketch,
2 MB LittleFS), so neither overwrites the other's settings: the Wi-Fi
credentials (SDK area) and the MQTT config of the Arduino firmware
(LittleFS) survive the round trip.

1. Build the rollback image first, while you are sure it matches what is
   installed (repository root, same `build_flags` as before):

   ```sh
   pio run -e nodemcuv2-ota
   ```

2. Switch to ESPHome via the Arduino OTA (no password):

   ```sh
   cd esphome
   esphome compile hood-control.yaml
   python3 ~/.platformio/packages/framework-arduinoespressif8266/tools/espota.py \
     -i hood-control.local -f .esphome/build/hood-control/.pioenvs/hood-control/firmware.bin
   ```

   Home Assistant discovers the new ESPHome device (API encryption key from
   `secrets.yaml`). The MQTT entities become unavailable meanwhile, and
   automations using them do nothing.

3. Test: `esphome logs hood-control.yaml` shows `Synchronised with the hood`
   and every LED change. Compare with the front panel, then switch speed,
   light and timer from Home Assistant.

4. Switch back via the ESPHome OTA:

   ```sh
   esphome upload hood-control.yaml --device hood-control.local \
     --file ../.pio/build/nodemcuv2-ota/firmware.bin
   ```

   Then delete the ESPHome device in Home Assistant; the MQTT entities come
   back on their own.

Safety nets if something goes wrong:

- After 10 failed boots in a row ESPHome starts in *safe mode* with only
  Wi-Fi and OTA, so step 4 still works.
- Without Wi-Fi, ESPHome opens the `HoodControl-Setup` hotspot; its portal
  at `http://192.168.4.1` also accepts a firmware upload.
- Until the firmware drives them, the key outputs are held LOW by R7/R8, so
  a crashing firmware cannot press keys.
- Last resort: serial flashing via J2 with J5 set to FLASH, see
  [OPERATIONS.md](../docs/OPERATIONS.md) – this requires opening the hood.

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
