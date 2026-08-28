#include <Arduino.h>
#include "VictronBLE.h"

VictronBLE victron;

// Callback — receives a VictronDevice*, switch on deviceType
void onVictronData(const VictronDevice *dev)
{

Serial.printf("Solar %s: %.2fV %.2fA %dW\n",
                  dev->name,
                  dev->solar.batteryVoltage,
                  dev->solar.batteryCurrent,
                  (int)dev->solar.panelPower);

  if (dev->deviceType == DEVICE_TYPE_SOLAR_CHARGER)
  {
    Serial.printf("Solar %s: %.2fV %.2fA %dW\n",
                  dev->name,
                  dev->solar.batteryVoltage,
                  dev->solar.batteryCurrent,
                  (int)dev->solar.panelPower);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up victronBLE monitor...");

  victron.begin(5); // 5 second scan duration
  victron.setCallback(onVictronData);

  // Add your device (replace with your MAC and key)
  boolean success = victron.addDevice(
      "SmartSolar-Mock",                  // Name
      "E8:DB:84:1C:FF:7A",                // MAC address
      "0123456789abcdef0123456789abcdef" // Encryption key
      // DEVICE_TYPE_SOLAR_CHARGER           // Device type (optional, auto-detected)
  );

  if (!success)
  {
    Serial.println("Failed to add device. Check MAC and key.");
  }
  else{
    Serial.println("Device added successfully. Awaiting data...");
  }

}

void loop()
{
  victron.loop(); // Non-blocking, returns immediately
  delay(100); 
}