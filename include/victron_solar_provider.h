#pragma once

#include "VictronBLE.h"
#include "solar_provider.h"

class VictronSolarProvider final : public SolarProvider {
public:
  VictronSolarProvider();

  bool begin() override;
  void update() override;
  bool latest(SolarSnapshot& snapshot) const override;

private:
  static void onData(const VictronDevice* device);
  static VictronSolarProvider* activeProvider_;

  VictronBLE victron_;
  SolarSnapshot snapshot_;
  bool configured_;
};
