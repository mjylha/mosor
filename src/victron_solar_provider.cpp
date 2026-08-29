#include "victron_solar_provider.h"

VictronSolarProvider* VictronSolarProvider::activeProvider_ = nullptr;

VictronSolarProvider::VictronSolarProvider() : configured_(false) {}

bool VictronSolarProvider::begin() {
  activeProvider_ = this;
  if (!victron_.begin(5)) {
    return false;
  }
  victron_.setCallback(onData);

  configured_ = victron_.addDevice(
      "SmartSolar",
      "E8:DB:84:1C:FF:7A",
      "0123456789abcdef0123456789abcdef",
      DEVICE_TYPE_SOLAR_CHARGER);
  return configured_;
}

void VictronSolarProvider::update() {
  // The BLE library receives advertisements through its callback.
  victron_.loop();
  if (snapshot_.status == SolarDataStatus::Fresh &&
      millis() - snapshot_.updatedAt > 10000) {
    snapshot_.status = SolarDataStatus::Stale;
  }
}

bool VictronSolarProvider::latest(SolarSnapshot& snapshot) const {
  snapshot = snapshot_;
  return snapshot_.status == SolarDataStatus::Fresh ||
         snapshot_.status == SolarDataStatus::Stale;
}

void VictronSolarProvider::onData(const VictronDevice* device) {
  if (activeProvider_ == nullptr || device == nullptr) {
    return;
  }

  if (device->deviceType != DEVICE_TYPE_SOLAR_CHARGER || !device->dataValid) {
    return;
  }

  activeProvider_->snapshot_.batteryVoltage = device->solar.batteryVoltage;
  activeProvider_->snapshot_.batteryCurrent = device->solar.batteryCurrent;
  activeProvider_->snapshot_.panelPower = device->solar.panelPower;
  activeProvider_->snapshot_.chargerState = device->solar.chargeState;
  activeProvider_->snapshot_.errorCode = device->solar.errorCode;
  activeProvider_->snapshot_.updatedAt = device->lastUpdate;
  activeProvider_->snapshot_.status = device->solar.errorCode == 0
      ? SolarDataStatus::Fresh
      : SolarDataStatus::Error;
}
