# Mosor solar monitor

Mosor is ESP32 firmware for monitoring a Victron SmartSolar charger. It can
show readings on a small SSD1306 OLED and publish them to MQTT. A deterministic
mock provider is also available for trying the firmware without a Victron
device.

## What you need

For a real installation:

- An ESP32 development board supported by PlatformIO
- A Victron SmartSolar device supported by the `victronble` library
- The SmartSolar device name, Bluetooth MAC address, and encryption key
- Optionally, a 0.96-inch 128x64 SSD1306 I2C OLED
- A USB cable and a computer with PlatformIO

The OLED is optional. Without it, readings are still printed over serial and
can be published through MQTT.

## Quick start: Victron SmartSolar

### 1. Install PlatformIO

Install [PlatformIO](https://platformio.org/) through PlatformIO IDE for
VS Code, or install the PlatformIO Core CLI. Confirm that the `pio` command is
available:

```sh
pio --version
```

Clone this repository and run the remaining commands from its directory.

### 2. Connect the OLED (optional)

The firmware uses I2C address `0x3C`, SDA on GPIO 21, and SCL on GPIO 22:

| OLED pin | ESP32 pin |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

The display uses the ESP32's 3.3 V supply. The current firmware does not use
the OLED reset pin.

### 3. Create local settings

Copy the example settings file. The local file is ignored by git and is the
right place for device identifiers, Wi-Fi credentials, MQTT credentials, and
other secrets:

```sh
cp include/victron_device_settings.example.h \
   include/victron_device_settings.local.h
```

Edit `include/victron_device_settings.local.h` and replace the placeholder
values:

```cpp
#define MOSOR_VICTRON_DEVICE_NAME "MySmartSolar"
#define MOSOR_VICTRON_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"
#define MOSOR_VICTRON_ENCRYPTION_KEY "0123456789abcdef0123456789abcdef"
```

The device name, MAC address, and encryption key must match the SmartSolar
device. Keep this file private; do not commit it or paste its contents into
issues or pull requests.

### 4. Build and upload

Build the real-device environment:

```sh
pio run -e esp32dev-victron
```

Connect the ESP32 over USB, then upload:

```sh
pio run -e esp32dev-victron -t upload
```

Open the serial monitor at 115200 baud:

```sh
pio device monitor -b 115200
```

On startup, the firmware initializes MQTT and the OLED, starts the Victron BLE
provider, and prints readings similar to:

```text
Solar: 13.20V 1.25A 42W state:3 status:1
```

## Try it without a Victron device

The default `esp32dev` environment uses a deterministic mock provider:

```sh
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

The mock generates a smooth day/night panel-power curve, battery readings, and
bounded jitter. Its seed is set in `src/main.cpp`, so runs are reproducible.
This environment is useful for checking the display and serial output before
connecting real hardware.

## Configuration

Configuration is defined in `include/victron_device_settings.local.h`. The
example file contains every supported setting:

| Setting | Purpose |
| --- | --- |
| `MOSOR_VICTRON_DEVICE_NAME` | SmartSolar Bluetooth device name |
| `MOSOR_VICTRON_MAC_ADDRESS` | SmartSolar Bluetooth MAC address |
| `MOSOR_VICTRON_ENCRYPTION_KEY` | Victron BLE encryption key |
| `MQTT_ENABLED` | Set to `"1"` or `"true"` to enable MQTT |
| `WIFI_SSID` / `WIFI_PASSWORD` | Wi-Fi network credentials |
| `MQTT_URL` | Broker host, optional scheme, and optional port |
| `MQTT_URL_IS_TLS` | Set to `"1"` or `"true"` for TLS |
| `MQTT_TOPIC_PREFIX` | Prefix for the published topic |
| `MQTT_USER` / `MQTT_PASSWORD` | Optional MQTT credentials |

`MQTT_URL` accepts a host with an optional port, `mqtt://host:port`, or
`mqtts://host:port`. If no port is provided, the firmware uses 1883 without
TLS and 8883 with TLS. The topic is
`<MQTT_TOPIC_PREFIX>/solar`; a missing trailing slash is added automatically.

The built-in defaults leave MQTT disabled. If MQTT is enabled but Wi-Fi or the
broker URL is empty, the firmware reports the configuration problem over
serial and continues without MQTT.

### MQTT payload

When connected, Mosor publishes a retained JSON snapshot every five seconds:

```json
{
  "batteryVoltage": 13.2,
  "batteryCurrent": 1.25,
  "panelPower": 42.0,
  "chargerState": 3,
  "errorCode": 0,
  "status": 1,
  "updatedAt": 123456
}
```

The `status` values are `0` unavailable, `1` fresh, `2` stale, and `3` error.
`updatedAt` is the provider's millisecond timestamp, not a wall-clock time.

For TLS connections, the current firmware calls `setInsecure()`, so the
broker certificate is not validated. Use a trusted network or add certificate
validation before relying on TLS for broker identity.

## OLED display and readings

The OLED shows:

- A solar icon and blinking energy-flow arrow
- Current panel power
- Battery gauge and estimated battery percentage
- Battery voltage and current
- Charger state and error code
- Data status and the age of the latest reading

The battery percentage is an estimate derived from voltage. Voltage is affected
by charging current, load, temperature, and surface charge, so it should not be
treated as a precision state-of-charge measurement. The display can also show
stale data when the last valid Victron reading is more than ten seconds old.

## Troubleshooting

### Build or upload problems

- Confirm that PlatformIO is installed and run commands from the repository
  root.
- Use `pio run -e esp32dev-victron` for a real SmartSolar device and
  `pio run -e esp32dev` for the mock.
- Check the selected USB port if upload cannot find the ESP32. PlatformIO can
  list detected devices with `pio device list`.
- Open the monitor at 115200 baud after uploading.

### Victron data is unavailable

- Confirm that the local settings file exists at
  `include/victron_device_settings.local.h`.
- Check the device name, MAC address, and encryption key carefully.
- Keep the ESP32 close to the SmartSolar device during initial testing.
- Look for `Failed to initialize solar provider.` in the serial output.
- If a reading stops updating, the provider marks it stale after ten seconds.

### OLED is blank

At startup, Mosor checks whether an I2C device acknowledges address `0x3C`.
`No I2C device acknowledged` usually means a power, ground, SDA/SCL,
connector, address-selection, or failed-module problem.

Check the following:

1. Power the OLED from 3.3 V and connect a common ground.
2. Connect SDA to GPIO 21 and SCL to GPIO 22.
3. Confirm that the module is a 128x64 SSD1306-compatible display.
4. Check the module's I2C address; the firmware currently expects `0x3C`.

The firmware uses a 50 ms I2C timeout and retries OLED initialization every
five minutes. If the address acknowledges but SSD1306 initialization fails,
check the display controller and wiring. The current wiring does not use the
OLED reset pin.

### Wi-Fi or MQTT does not connect

- Set `MQTT_ENABLED` to `"1"` or `"true"`.
- Check `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_URL`, and the MQTT credentials.
- Confirm that the TLS setting and broker port agree.
- Watch the serial output for Wi-Fi events and MQTT connection state.
- Remember that MQTT is optional; OLED and serial monitoring continue when
  MQTT is disabled or unavailable.

## Project layout

- `src/main.cpp` - application startup and update loop
- `src/display.cpp` - OLED rendering and battery estimate
- `src/mqtt.cpp` - Wi-Fi/MQTT setup and publishing
- `src/mock_solar_provider.cpp` - deterministic demo data
- `src/victron_solar_provider.cpp` - Victron BLE integration
- `include/victron_device_settings.example.h` - local configuration template
- `platformio.ini` - PlatformIO environments and dependencies
