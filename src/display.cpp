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
constexpr float batteryEmptyVoltage = 12.0f;
constexpr float batteryFullVoltage = 14.0f;


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

uint8_t estimatedBatteryPercent(float voltage) {
  const float range = batteryFullVoltage - batteryEmptyVoltage;
  if (voltage <= batteryEmptyVoltage) {
    return 0;
  }
  if (voltage >= batteryFullVoltage) {
    return 100;
  }
  return static_cast<uint8_t>(
      ((voltage - batteryEmptyVoltage) / range) * 100.0f + 0.5f);
}


void drawBatteryAddSolarInput(const SolarSnapshot& snapshot) {
  const uint8_t percent = estimatedBatteryPercent(snapshot.batteryVoltage);
  constexpr int gaugeX = 1;
  constexpr int gaugeY = 1;
  constexpr int gaugeWidth = 50;
  constexpr int gaugeHeight = 14;

  display.drawRect(gaugeX, gaugeY, gaugeWidth, gaugeHeight, SSD1306_WHITE);
  display.fillRect(gaugeX + gaugeWidth, gaugeY + 4, 3, 6, SSD1306_WHITE);
  const int fillWidth = (gaugeWidth - 4) * percent / 100;
  if (fillWidth > 0) {
    display.fillRect(gaugeX + 2, gaugeY + 2, fillWidth, gaugeHeight - 4,
                     SSD1306_WHITE);
  }

  const int sunX = gaugeX + gaugeWidth + 16;
  const int sunY = gaugeY + gaugeHeight / 2;
  display.fillRect(sunX, sunY + 5, 1, 2, SSD1306_WHITE); // ray bottom
  display.fillRect(sunX, sunY - 6, 1, 2, SSD1306_WHITE); // ray top
  display.fillRect(sunX - 6, sunY, 2, 1, SSD1306_WHITE); // ray left
  display.fillRect(sunX + 5, sunY, 2, 1, SSD1306_WHITE); // ray right
  display.fillCircle(sunX, sunY, 3, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(77, 0);
  display.print(snapshot.panelPower, 0);
  display.println("W");

  display.setTextSize(2);
  display.print(percent);
  display.print("% " );
  display.setTextSize(1);
  display.print(snapshot.batteryVoltage, 2);
  display.println("V ");
  display.print("         ");
  display.print(snapshot.batteryCurrent, 2);
  display.println("A");

  display.setTextSize(1);
  display.print("State: ");
  display.print(snapshot.chargerState);
  display.print(" Err: ");
  display.println(snapshot.errorCode);
  display.print(statusName(snapshot.status));
  display.print(" ");
  display.print((millis() - snapshot.updatedAt) / 1000);
  display.println(" s");
  display.display();

  
  
  //display.println("%");
  //display.setCursor(60, 9);
  //display.print(snapshot.batteryVoltage, 1);
  //display.print("V");
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
  drawBatteryAddSolarInput(snapshot);
  
}
