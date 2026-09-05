# Mosor solar monitor

The firmware reads solar data through the project-owned `SolarProvider`
interface in `include/solar_provider.h`. Consumers should use `SolarSnapshot`
instead of depending on the Victron BLE library.

The default environment uses a deterministic mock:

```sh
pio run -e esp32dev
pio device monitor
```

The mock produces a smooth day/night curve with bounded jitter. Its seed is
set in `src/main.cpp`, so the same run can be reproduced.

## OLED display

The firmware displays the current `SolarSnapshot` on a 0.96 inch I2C
128x64 SSD1306 OLED. The default I2C address is `0x3C`.

Connect the display to an ESP32 development board as follows:

| OLED pin | ESP32 pin |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

The display shows battery voltage, battery current, panel power, charger state,
error code, data status, and the age of the latest reading. If the OLED is not
connected, the firmware continues to report readings over serial.


When connected to the real SmartSolar device, build the Victron adapter
environment:

```sh
pio run -e esp32dev-victron
```

Before flashing that environment, create a local settings file that is ignored by
git:

```sh
cp include/victron_device_settings.example.h include/victron_device_settings.local.h
```

This example file is the template: when copied to
`include/victron_device_settings.local.h`, it takes precedence over the default
settings in `include/victron_device_settings.h`. Then edit the local file and
replace the device name, MAC address, and encryption key with your own values.
The project will prefer the local file automatically when flashing the Victron
build.

When `MQTT_ENABLED` is `"1"` or `"true"`, the firmware connects to
`WIFI_SSID`/`WIFI_PASSWORD` and publishes a retained JSON `SolarSnapshot` every
five seconds to `<MQTT_TOPIC_PREFIX>/solar`. Set `MQTT_URL_IS_TLS` to `"1"` or
`"true"` for TLS (the configured broker certificate is not validated), or to
`"0"` for plain MQTT. `MQTT_URL` accepts `host:port`, `mqtt://host:port`, or
`mqtts://host:port`; ports default to 1883 or 8883 based on the TLS setting.

## Display troubleshooting and details

At startup, the firmware checks whether an I2C device acknowledges at `0x3C`.
`No I2C device acknowledged` usually indicates a power, ground, SDA/SCL,
connector, address-selection, or failed-module problem. If the address
acknowledges but SSD1306 initialization fails, check that the module is really
an SSD1306-compatible 128x64 display.

For a black display, first power the module from 3.3 V, verify common ground
and the GPIO 21/22 connections, and inspect the header and solder joints. A
replacement display working on the same board, wiring, firmware, and supply
is strong evidence that the original module failed. If the original still
does not acknowledge at `0x3C` after those checks, software cannot repair the
module; replacement is the practical remedy.

The firmware sets a 50 ms I2C timeout and re-sends the SSD1306 initialization
sequence every five minutes. This can recover a display controller that has
stopped responding while the ESP32 and MQTT connection remain healthy. The
current wiring does not use the OLED reset pin; for installations where the
display must recover from severe electrical disturbances, wiring the module's
reset input to an ESP32 GPIO and passing that pin to the SSD1306 constructor
provides a stronger hardware reset.