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
build. Displays, transmission, and other consumers can be added around
`SolarSnapshot` without changing provider selection.
