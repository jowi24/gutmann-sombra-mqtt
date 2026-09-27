🇬🇧 **English** · 🇩🇪 [Deutsch](OPERATIONS.de.md)

# Operation, diagnostics and flashing

This document summarises the hardware data determined on 16 August 2026,
the observed failure pattern, and how to operate firmware version 0.0.7 and
later.

## Hardware

| Property | Value |
| --- | --- |
| Platform | ESP8266 |
| Detected chip | ESP8266EX |
| Features | Wi-Fi, up to 160 MHz |
| Crystal | 26 MHz |
| MAC address | `68:c6:3a:88:b4:ba` |
| PlatformIO board | `nodemcuv2` |
| Circuit design | ESP8266-12E module |
| Serial adapter | `/dev/cu.usbserial-AD025JRF` or `/dev/tty.usbserial-AD025JRF` |
| Firmware baud rate | 115200 baud |
| ROM boot output | 74880 baud |

The MAC address uniquely identifies this particular device. The address
`192.168.178.48` used during diagnosis was assigned dynamically via DHCP and
may change.

## Original failure pattern

- The device is regularly disconnected from power once or twice a day.
- This worked for several weeks.
- Afterwards the device stayed offline in MQTT for about five days.
- After a restart, Pi-hole showed no DNS query for `mqtt.local`.
- The known DHCP leases did not belong to the device; its MAC address was
  not present.
- After removing it and starting it at a location with stronger Wi-Fi, the
  device connected again.

The test outside the installation location proves that the ESP8266, flash,
firmware configuration, Wi-Fi and MQTT work in principle. It says nothing
about the radio quality at the installation location.

## Probable cause

The old firmware used a blocking call to `WiFiManager::autoConnect()`. If
the first Wi-Fi connection at power-up failed because of the weak signal,
the device fell into a permanently blocking configuration portal. The later
reconnect logic in `loop()` was never reached.

Not seeing the setup hotspot at the installation location does not rule out
this state: the ESP8266's own signal can be heavily attenuated by the same
location.

The built-in power supply also remains a possible cause. Successful
operation over USB can hide a weak or unstable 3.3 V supply.

## Behaviour from firmware 0.0.7

1. The firmware starts the hood control without waiting for Wi-Fi.
2. It tries to reach the stored network every ten seconds.
3. After 20 seconds without a connection it additionally starts the
   password-protected hotspot `HoodControl-Setup`.
4. The portal is available at `http://192.168.4.1`.
5. While the hotspot is active, the device retries the stored network every
   30 seconds.
6. After one minute of stable Wi-Fi the hotspot is shut down.
7. MQTT connection attempts use exponential backoff from five seconds up to
   five minutes.
8. After an MQTT reconnect, discovery and all hood states are republished.

## Configuration hotspot

| Property | Value |
| --- | --- |
| SSID | `HoodControl-Setup` |
| Password | `REMOVED` |
| Portal | `http://192.168.4.1` |

The password must be at least eight characters long, otherwise the ESP8266
cannot start the hotspot as a protected network.

After saving new Wi-Fi or MQTT settings the device restarts in a controlled
way.

## MQTT

The default prefix is `home/kitchen/hood`.

### Availability

| Topic | Meaning |
| --- | --- |
| `$online` | retained availability, `true` or last will `false` |
| `$firmware` | firmware version |
| `$ip` | current DHCP address |
| `$wifi_rssi` | Wi-Fi signal strength in dBm |
| `$uptime` | uptime in seconds |
| `$reset_reason` | ESP8266 reset reason |
| `$last_error` | last detected error |

### Extended diagnostics

The retained topic `diagnostics` contains, among other things:

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

The non-retained topic `events` delivers up to 16 buffered events as soon as
MQTT is reachable again, for example Wi-Fi drop-outs, reconnect attempts,
portal start and MQTT errors.

### Interpreting RSSI

| RSSI | Assessment |
| --- | --- |
| better than -60 dBm | good |
| -60 to -70 dBm | usable |
| -70 to -80 dBm | weak |
| worse than -80 dBm | very unstable |

What matters is the value at the actual installation location with the hood
closed.

## USB flashing

### Normal PlatformIO upload

```sh
pio run -e nodemcuv2 -t upload
```

The automatic switch into the bootloader did not work reliably with the
serial adapter at hand and ended with:

```text
Failed to connect to ESP8266: Timed out waiting for packet header
```

### Manual procedure that worked

1. Hold down the single button on the module.
2. Connect the serial adapter or start an upload with automatic reset.
3. Keep the button pressed while the connection is established.
4. Once writing has started, the button can be released.
5. After flashing, reconnect the adapter without the button pressed so that
   the firmware starts normally.

Direct upload used:

```sh
esptool --chip esp8266 \
  --port /dev/cu.usbserial-AD025JRF \
  --baud 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash 0x0 .pio/build/nodemcuv2/firmware.bin
```

The successful run detected the ESP8266EX with MAC address
`68:c6:3a:88:b4:ba`, wrote 437,760 bytes and then verified the flash hash.

Pressing the button at power-up most likely pulls GPIO0 LOW and thereby
activates the ROM bootloader. Without the button pressed, the application
firmware starts.

(On the PCB rev. 0.2/0.3 the jumper J5 selects RUN or FLASH instead – see
[CIRCUIT.md](CIRCUIT.md), section 10.)

## Diagnostic procedure after another failure

1. Check whether `home/kitchen/hood/$online` has changed to `false`.
2. Look for `HoodControl-Setup` and open `192.168.4.1` if needed.
3. Search Pi-hole or the DHCP server for MAC address `68:c6:3a:88:b4:ba`.
4. Check the MQTT topics `$wifi_rssi`, `$last_error`, `$reset_reason`,
   `diagnostics` and `events`.
5. With USB access, open the serial monitor at 115200 baud.
6. If the USB-serial port is visible but not even the ESP ROM bootloader
   sends any data, check the supply, reset and boot pins.

## Electrical check

If the device only works reliably on USB but not in the hood, measure
against GND:

| Pin | Expected level |
| --- | --- |
| VCC | about 3.3 V |
| EN/CH_PD | HIGH, about 3.3 V |
| RST | HIGH, about 3.3 V |
| GPIO0 | HIGH on normal start |
| GPIO2 | HIGH on normal start |
| GPIO15 | LOW on normal start |

Voltage dips during Wi-Fi transmit bursts are also relevant. The firmware
uses maximum Wi-Fi transmit power and disables Wi-Fi sleep mode, which makes
a stable power supply especially important.
