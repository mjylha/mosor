#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <cstring>

#include "display.h"

namespace {
constexpr uint8_t displayWidth = 128;
constexpr uint8_t displayHeight = 64;
constexpr uint8_t displayAddress = 0x3C;
constexpr int i2cSda = 21;
constexpr int i2cScl = 22;
constexpr uint32_t recoveryInterval = 300000;
constexpr uint32_t pixelShiftInterval = 60000;
constexpr uint32_t blankingInterval = 300000;
constexpr uint32_t blankingDuration = 2000;
constexpr size_t displayBufferSize = displayWidth * displayHeight / 8;

Adafruit_SSD1306 display(displayWidth, displayHeight, &Wire, -1);
bool displayReady = false;
bool displayBlanked = false;
uint32_t lastRecovery = 0;
uint32_t lastPixelShift = 0;
uint32_t lastBlanking = 0;
uint32_t blankingStarted = 0;
uint8_t shiftedBuffer[displayBufferSize];

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

struct BatteryVoltagePoint {
  float voltage;
  uint8_t percent;
};

// Voltage is a poor proxy for SOC while charging, but this curve follows the
// battery monitor's high-voltage plateau better than a linear conversion.
constexpr BatteryVoltagePoint batteryCurve[] = {
    {12.0f, 0},   {12.2f, 15}, {12.4f, 35}, {12.6f, 55},
    {12.8f, 72},  {13.0f, 86}, {13.2f, 95}, {13.4f, 98},
    {13.6f, 100},
};

uint8_t estimatedBatteryPercent(float voltage) {
  if (voltage <= batteryCurve[0].voltage) {
    return 0;
  }
  constexpr size_t pointCount = sizeof(batteryCurve) / sizeof(batteryCurve[0]);
  if (voltage >= batteryCurve[pointCount - 1].voltage) {
    return 100;
  }

  for (size_t i = 1; i < pointCount; ++i) {
    const BatteryVoltagePoint& lower = batteryCurve[i - 1];
    const BatteryVoltagePoint& upper = batteryCurve[i];
    if (voltage <= upper.voltage) {
      const float linearFraction =
          (voltage - lower.voltage) / (upper.voltage - lower.voltage);
      // Ease between calibration points so the estimate does not change at a
      // constant rate throughout each voltage interval.
      const float fraction =
          linearFraction * linearFraction * (3.0f - 2.0f * linearFraction);
      return static_cast<uint8_t>(
          lower.percent + (upper.percent - lower.percent) * fraction + 0.5f);
    }
  }

  return 100;
}

void shiftDisplayBuffer(uint8_t xOffset, uint8_t yOffset) {
  uint8_t* buffer = display.getBuffer();
  memset(shiftedBuffer, 0, sizeof(shiftedBuffer));

  for (uint8_t y = 0; y < displayHeight - yOffset; ++y) {
    for (uint8_t x = 0; x < displayWidth - xOffset; ++x) {
      const size_t sourceIndex = x + (y / 8) * displayWidth;
      if ((buffer[sourceIndex] & (1 << (y & 7))) != 0) {
        const uint8_t shiftedY = y + yOffset;
        const size_t targetIndex =
            (x + xOffset) + (shiftedY / 8) * displayWidth;
        shiftedBuffer[targetIndex] |= 1 << (shiftedY & 7);
      }
    }
  }

  memcpy(buffer, shiftedBuffer, sizeof(shiftedBuffer));
}

void maintainDisplayProtection(uint32_t now) {
  if (displayBlanked) {
    if (now - blankingStarted >= blankingDuration) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displayBlanked = false;
      lastBlanking = now;
    }
    return;
  }

  if (now - lastBlanking >= blankingInterval) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displayBlanked = true;
    blankingStarted = now;
  }
}


void drawBatteryAddSolarInput(const SolarSnapshot& snapshot, bool networkProblem) {
  const uint8_t percent = estimatedBatteryPercent(snapshot.batteryVoltage);
  
  constexpr int bigRowheight = 14;
  const int sunX = 5;
  const int sunY = 5;

  constexpr int gaugeX = 1;
  constexpr int gaugeWidth = 50;
  const int gaugeY = bigRowheight + 2;
  
  // sun
  display.fillRect(sunX, sunY + 5, 1, 2, SSD1306_WHITE); // ray bottom
  display.fillRect(sunX, sunY - 6, 1, 2, SSD1306_WHITE); // ray top
  display.fillRect(sunX - 6, sunY, 2, 1, SSD1306_WHITE); // ray left
  display.fillRect(sunX + 5, sunY, 2, 1, SSD1306_WHITE); // ray right
  display.fillCircle(sunX, sunY, 3, SSD1306_WHITE);

  // Solar energy flow from the sun to the panel.
  constexpr int panelX = 28;
  constexpr int panelY = 2;
  constexpr int panelWidth = 7;
  constexpr int panelHeight = 12;
  display.drawRect(panelX, panelY, panelWidth, panelHeight, SSD1306_WHITE);
  display.drawLine(panelX + 3, panelY, panelX + 3, panelY + panelHeight - 1,
                   SSD1306_WHITE);
  display.drawLine(panelX + 1, panelY + 4, panelX + panelWidth - 2,
                   panelY + 4, SSD1306_WHITE);
  display.drawLine(panelX + 1, panelY + 8, panelX + panelWidth - 2,
                   panelY + 8, SSD1306_WHITE);

  if ((millis() / 500) % 2 == 0 /*&& snapshot.panelPower > 0*/) {
    constexpr int arrowY = 5;
    display.drawLine(sunX + 9, arrowY, panelX - 3, arrowY, SSD1306_WHITE);
    display.drawLine(sunX + 9, arrowY + 1, panelX - 3, arrowY + 1,
                     SSD1306_WHITE);
    display.drawLine(panelX - 3, arrowY, panelX - 6, arrowY - 2,
                     SSD1306_WHITE);
    display.drawLine(panelX - 3, arrowY + 1, panelX - 6, arrowY - 1,
                     SSD1306_WHITE);
    display.drawLine(panelX - 3, arrowY + 1, panelX - 6, arrowY + 3,
                     SSD1306_WHITE);
  }

  display.setTextSize(2);
  display.print("     ");
  display.print(snapshot.panelPower, 0);
  display.println("W");


  // battery gauge
  display.drawRect(gaugeX, gaugeY, gaugeWidth, bigRowheight, SSD1306_WHITE);
  display.fillRect(gaugeX + gaugeWidth, gaugeY + 4, 3, 6, SSD1306_WHITE);
  const int fillWidth = (gaugeWidth - 4) * percent / 100;
  if (fillWidth > 0) {
    display.fillRect(gaugeX + 2, gaugeY + 2, fillWidth, bigRowheight - 4,
                     SSD1306_WHITE);
  }

  display.setTextSize(2);
  display.print("     ");
  display.print(percent);
  display.println("% " );

  // additional battery info
  display.setTextSize(1);
  display.print(snapshot.batteryVoltage, 2);
  display.print("V ");
  display.print(snapshot.batteryCurrent, 2);
  display.println("A");

  // the rest
  display.setTextSize(1);
  display.print("State: ");
  display.print(snapshot.chargerState);
  display.print(" Err: ");
  display.println(snapshot.errorCode);
  if (networkProblem) {
    display.println("Network problem");
  } else {
    display.print(statusName(snapshot.status));
    display.print(" ");
    display.print((millis() - snapshot.updatedAt) / 1000);
    display.println(" s");
  }

  // burn-in prevention
  const uint32_t now = millis();
  if (now - lastPixelShift >= pixelShiftInterval) {
    lastPixelShift = now;
    const uint8_t shift = (now / pixelShiftInterval) % 2;
    shiftDisplayBuffer(shift, shift);
  }
  display.display();
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
  displayBlanked = false;
  lastPixelShift = millis();
  lastBlanking = lastPixelShift;
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
  maintainDisplayProtection(now);
  if (now - lastRecovery >= recoveryInterval) {
    lastRecovery = now;
    initialize();
  }
}

void displayShow(const SolarSnapshot& snapshot, bool networkProblem) {
  if (!displayReady) {
    return;
  }
  if (displayBlanked) {
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  drawBatteryAddSolarInput(snapshot, networkProblem);
  
}
