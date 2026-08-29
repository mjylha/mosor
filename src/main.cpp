#include <Arduino.h>
#include "solar_provider.h"

#if defined(SOLAR_PROVIDER_VICTRON)
#include "victron_solar_provider.h"
VictronSolarProvider solar;
#else
#include "mock_solar_provider.h"
MockSolarProvider solar(42);
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("Starting solar monitor...");

  if (!solar.begin()) {
    Serial.println("Failed to initialize solar provider.");
  }
}

void loop() {
  solar.update();

  SolarSnapshot snapshot;
  if (solar.latest(snapshot)) {
    Serial.printf("Solar: %.2fV %.2fA %.0fW state:%u status:%u\n",
                  snapshot.batteryVoltage,
                  snapshot.batteryCurrent,
                  snapshot.panelPower,
                  snapshot.chargerState,
                  static_cast<unsigned>(snapshot.status));
  }

  delay(100);
}
