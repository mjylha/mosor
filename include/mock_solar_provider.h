#pragma once

#include "solar_provider.h"

class MockSolarProvider final : public SolarProvider {
public:
  explicit MockSolarProvider(uint32_t seed = 1);

  bool begin() override;
  void update() override;
  bool latest(SolarSnapshot& snapshot) const override;

private:
  uint32_t nextRandom();
  float noise(float amplitude);

  SolarSnapshot snapshot_;
  uint32_t seed_;
  uint32_t startedAt_;
  bool started_;
};
