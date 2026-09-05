#include <Arduino.h>

#include "display.h"
#include "mqtt.h"
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

  mqttBegin();
  displayBegin();

  if (!solar.begin()) {
    Serial.println("Failed to initialize solar provider.");
  }
}

void loop() {
  solar.update();
  mqttUpdate();
  displayMaintain();

  SolarSnapshot snapshot;
  if (solar.latest(snapshot)) {
    Serial.printf("Solar: %.2fV %.2fA %.0fW state:%u status:%u\n",
                  snapshot.batteryVoltage,
                  snapshot.batteryCurrent,
                  snapshot.panelPower,
                  snapshot.chargerState,
                  static_cast<unsigned>(snapshot.status));
    displayShow(snapshot);
    mqttPublish(snapshot);
  }

  delay(100);
}
