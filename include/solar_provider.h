#pragma once

#include <Arduino.h>

enum class SolarDataStatus : uint8_t {
  Unavailable,
  Fresh,
  Stale,
  Error
};

// The most recent solar/charger reading from a source. Providers update this
// struct whenever they receive fresh telemetry or detect that the data is stale.
struct SolarSnapshot {
  float batteryVoltage = 0.0f;
  float batteryCurrent = 0.0f;
  float panelPower = 0.0f;
  uint8_t chargerState = 0;
  uint8_t errorCode = 0;
  uint32_t updatedAt = 0;
  SolarDataStatus status = SolarDataStatus::Unavailable;
};

// Common interface for solar data sources. Implementations may read from a
// mock, a BLE device, or any other backend, but they all expose the same
// latest snapshot to consumers.
class SolarProvider {
public:
  virtual ~SolarProvider() = default;
  virtual bool begin() = 0;
  virtual void update() = 0;

  // Copies the current reading into `snapshot` and reports whether usable data is
  // available. A return value of `true` means the provider has a valid (fresh or
  // stale) reading to consume; `false` means the provider is not initialized or no
  // sample is currently available.
  virtual bool latest(SolarSnapshot& snapshot) const = 0;
};
