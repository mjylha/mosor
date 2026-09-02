#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "solar_provider.h"

namespace {
constexpr uint8_t displayWitdh = 128;
constexpr uint8_t displayHeight = 64;
constexpr uint8_t displayAddress = 0x3C;
constexpr int i2cSda = 21;
constexpr int i2cScl = 22;

Adafruit_SSD1306 display(displayWitdh, displayHeight, &Wire, -1);
bool displayReady = false;

const char* statusName(SolarDataStatus status) {
  switch (status) {
    case SolarDataStatus::Fresh:
      return "Fresh";
    case SolarDataStatus::Stale:
      return "Stale";
    case SolarDataStatus::Error:
      return "Error";
    case SolarDataStatus::Unavailable:
    default:
      return "None";
  }
}

void showSnapshot(const SolarSnapshot& snapshot) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Batt: ");
  display.print(snapshot.batteryVoltage, 2);
  display.println(" V");
  display.print("Curr: ");
  display.print(snapshot.batteryCurrent, 2);
  display.println(" A");
  display.print("Panel: ");
  display.print(snapshot.panelPower, 0);
  display.println(" W");
  display.print("State: ");
  display.print(snapshot.chargerState);
  display.print(" Err: ");
  display.println(snapshot.errorCode);
  display.print("Status: ");
  display.println(statusName(snapshot.status));
  display.print("Age: ");
  display.print((millis() - snapshot.updatedAt) / 1000);
  display.println(" s");
  display.display();
}
}  // namespace

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

  Wire.begin(i2cSda, i2cScl);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, displayAddress);
  if (!displayReady) {
    Serial.println("Failed to initialize OLED display.");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    Serial.println("OLED display initialized.");
  }

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
    if (displayReady) {
      showSnapshot(snapshot);
    }
  }

  delay(100);
}
