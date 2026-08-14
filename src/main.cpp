#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define LED_BUILTIN 2

// https://randomnerdtutorials.com/esp32-bluetooth-guide/
// https://randomnerdtutorials.com/esp32-ble-server-client/ <-- this
int scanTime = 5; // in seconds
BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.printf("Advertised Device: %s %s \n", advertisedDevice.toString().c_str(), "foobar");
  }
};

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  BLEDevice::init("");
  setupBLEScan();
}

void setupBLEScan()
{
  Serial.println("Scanning BLE devices...");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{

  scanForBLEDevices();
  delay(2000);

  //  digitalWrite(LED_BUILTIN, HIGH);

  //   // wait for a second
  //   delay(1000);

  //   // turn the LED off by making the voltage LOW
  //   digitalWrite(LED_BUILTIN, LOW);

  //    // wait for a second
  //   delay(300);
}

void scanForBLEDevices()
{
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
}
