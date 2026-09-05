#include "victron_solar_provider.h"
#include "victron_device_settings.h"

VictronSolarProvider* VictronSolarProvider::activeProvider_ = nullptr;

VictronSolarProvider::VictronSolarProvider() : configured_(false) {}

bool VictronSolarProvider::begin() {
  activeProvider_ = this;
  if (!victron_.begin(5)) {
    return false;
  }
  victron_.setCallback(onData);
  // Increase minimum processing interval to reduce BLE processing frequency
  // and avoid any risk of overloading the Victron device.
  victron_.setMinInterval(2000); // 2000 ms == 2 seconds

  configured_ = victron_.addDevice(
      MOSOR_VICTRON_DEVICE_NAME,
      MOSOR_VICTRON_MAC_ADDRESS,
      MOSOR_VICTRON_ENCRYPTION_KEY,
      DEVICE_TYPE_SOLAR_CHARGER);
  return configured_;
}

void VictronSolarProvider::update() {
  // The BLE library receives advertisements through its callback.
  victron_.loop();
  if (snapshot_.status == SolarDataStatus::Fresh &&   millis() - snapshot_.updatedAt > 10000) {
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
