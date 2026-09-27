🇬🇧 **English** · 🇩🇪 [Deutsch](CHANGELOG.de.md)

# Changelog

All notable changes to the firmware are recorded in this document.

## 0.0.9 - 2026-09-26

Adaptation to the fabricated PCB (rev. 0.3), see
[`docs/CIRCUIT.md`](docs/CIRCUIT.md).

### Changed

- The key outputs GPIO12/14 are now always actively driven:
  HIGH = press, LOW = release. Previously they were switched to
  high-impedance to release, which leaves the inputs of level shifter U3
  floating and produces phantom key presses.
- GPIO12/14 are set LOW right at the start of `setup()`.
- LED pins (`ledPin1/2`) and key pins (`buttonPin1/2`) swapped – both
  channel pairs arrive swapped on the PCB compared to the perfboard version.

### Fixed

- Home Assistant discovery of the fan: `state_value_template` returned
  `on`/`off`, but Home Assistant compares the result against
  `payload_on`/`payload_off` (`1`/`0`), so the fan was permanently
  `unknown`. The template now returns `1`/`0`.
- Fan level `0` is reported as preset `None` and resets the preset instead
  of being discarded as an invalid preset.

## 0.0.8 - 2026-09-08

### Added

- Status LED on GPIO2 (D4, blue on-board LED of the ESP-12, active LOW).
  The blink pattern shows the boot and connection phase without a serial
  console: three short flashes at power-up, fast blinking while joining
  Wi-Fi, medium blinking while connecting to MQTT, slow blinking while the
  configuration portal is open, and a short heartbeat every three seconds
  once both are up.

## 0.0.7 - 2026-08-16

### Added

- Non-blocking Wi-Fi start with the hood control running in parallel.
- Password-protected fallback hotspot `HoodControl-Setup`.
- Configuration portal at `http://192.168.4.1`.
- Periodic Wi-Fi reconnection attempts in AP+STA mode.
- Unique MQTT client ID from device name and ESP8266 chip ID.
- MQTT diagnostics under `diagnostics`, `$wifi_rssi`, `$uptime`,
  `$reset_reason`, `$firmware`, `$ip` and `$last_error`.
- Buffered connection events under the MQTT topic `events`.
- Persistent boot stage for diagnosing incomplete start-ups.
- Wi-Fi, MQTT and error counters.
- Full republish of all hood states after an MQTT reconnect.

### Changed

- MQTT reconnects use exponential backoff up to five minutes.
- MQTT socket timeout reduced to three seconds.
- MQTT commands are subscribed with QoS 1.
- The fallback hotspot is switched off after one minute of stable Wi-Fi.
- Configuration changes trigger a controlled restart after a successful save.
- Errors reading or writing the configuration, and publish/subscribe
  errors, are logged visibly.

### Fixed

- A failed Wi-Fi start can no longer lock the firmware permanently in the
  blocking WiFiManager portal.
- State changes during an MQTT outage are resynchronised after reconnecting.
- A fixed MQTT client ID can no longer collide with a second installation of
  the same name.

### Validation

- Successful build for `nodemcuv2` and `nodemcuv2-ota`.
- RAM: 35,892 of 81,920 bytes (43.8 %).
- Flash: 433,608 of 1,044,464 bytes (41.5 %).
- Firmware written over USB and verified against the flash hash.

## 0.0.6 - 2026-07-14

- Wi-Fi auto-reconnect enabled.
- Maximum Wi-Fi transmit power set.
- Wi-Fi sleep mode disabled.
- Active Wi-Fi check added; MQTT connection attempts only while Wi-Fi is up.

## 0.0.5

- Firmware migrated to PlatformIO.
- Homie replaced by WiFiManager and PubSubClient.
- Home Assistant MQTT discovery added.
- Fan presets, light entity and OTA support added.
