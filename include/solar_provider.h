#pragma once

#include <Arduino.h>

enum class SolarDataStatus : uint8_t {
  Unavailable,
  Fresh,
  Stale,
  Error
};

struct SolarSnapshot {
  float batteryVoltage = 0.0f;
  float batteryCurrent = 0.0f;
  float panelPower = 0.0f;
  uint8_t chargerState = 0;
  uint8_t errorCode = 0;
  uint32_t updatedAt = 0;
  SolarDataStatus status = SolarDataStatus::Unavailable;
};

class SolarProvider {
public:
  virtual ~SolarProvider() = default;
  virtual bool begin() = 0;
  virtual void update() = 0;
  virtual bool latest(SolarSnapshot& snapshot) const = 0;
};
