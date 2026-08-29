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

When connected to the real SmartSolar device, build the Victron adapter
environment:

```sh
pio run -e esp32dev-victron
```

Before flashing that environment, replace the device name, MAC address, and
encryption key in `src/solar_provider.cpp`. Displays, transmission, and other
consumers can be added around `SolarSnapshot` without changing provider
selection.
