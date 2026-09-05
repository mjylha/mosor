#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "display.h"

namespace {
constexpr uint8_t displayWidth = 128;
constexpr uint8_t displayHeight = 64;
constexpr uint8_t displayAddress = 0x3C;
constexpr int i2cSda = 21;
constexpr int i2cScl = 22;
constexpr uint32_t recoveryInterval = 300000;

Adafruit_SSD1306 display(displayWidth, displayHeight, &Wire, -1);
bool displayReady = false;
uint32_t lastRecovery = 0;

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

bool devicePresent() {
  Wire.beginTransmission(displayAddress);
  return Wire.endTransmission() == 0;
}

bool initialize() {
  if (!devicePresent()) {
    displayReady = false;
    Serial.printf("No I2C device acknowledged at OLED address 0x%02X.\n",
                  displayAddress);
    return false;
  }

  displayReady = display.begin(SSD1306_SWITCHCAPVCC, displayAddress,
                               true, false);
  if (!displayReady) {
    Serial.println("OLED controller acknowledged, but SSD1306 initialization failed.");
    return false;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  Serial.println("OLED display initialized.");
  return true;
}
}  // namespace

void displayBegin() {
  Wire.begin(i2cSda, i2cScl);
  Wire.setTimeOut(50);
  initialize();
  lastRecovery = millis();
}

void displayMaintain() {
  const uint32_t now = millis();
  if (now - lastRecovery >= recoveryInterval) {
    lastRecovery = now;
    initialize();
  }
}

void displayShow(const SolarSnapshot& snapshot) {
  if (!displayReady) {
    return;
  }

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
