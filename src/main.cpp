#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define LED_BUILTIN 2

#define bleServerName "Klipsch The Fives"
static BLEUUID bmeServiceUUID("0000180a-0000-1000-8000-00805f9b34fb");
static BLEUUID characteristicUUID("00002a29-0000-1000-8000-00805f9b34fb");
static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *someCharacteristic;
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

// https://randomnerdtutorials.com/esp32-bluetooth-guide/
// https://randomnerdtutorials.com/esp32-ble-server-client/ <-- this
int scanTime = 5; // in seconds
BLEScan *pBLEScan;

// Prints all devices found during the scan.
class ScannerCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.printf("Advertised Device: %s %s \n", advertisedDevice.toString().c_str(), "foobar");
  }
};

// Scanning callback. Finds the specific server address.
class FindSpecificCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.getName() == bleServerName)
    {                                                                 // Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop();                             // Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); // Address of advertiser is the one we need
      doConnect = true;                                               // Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                           uint8_t *pData, size_t length, bool isNotify)
{
  Serial.printf("Got from notify: %s \n", (char *)pData);
  // temperatureChar = (char*)pData;
  // newTemperature = true;
}

void printCharacteristicInfo(BLERemoteCharacteristic *pCharacteristic)
{
  Serial.printf("    Characteristic UUID: %s", pCharacteristic->getUUID().toString().c_str());

  // Some BLE characteristic implementations expose a name via the UUID map, but not all do.
  // If the library exposes one, print it; otherwise just print that no name is available.
  const char *name = nullptr;
  if (pCharacteristic->getUUID().equals(BLEUUID((uint16_t)0x2A00)))
    name = "Device Name";
  else if (pCharacteristic->getUUID().equals(BLEUUID((uint16_t)0x2A01)))
    name = "Appearance";
  else if (pCharacteristic->getUUID().equals(BLEUUID((uint16_t)0x2A19)))
    name = "Battery Level";
  else if (pCharacteristic->getUUID().equals(BLEUUID((uint16_t)0x2A29)))
    name = "Manufacturer Name String";

  if (name != nullptr)
    Serial.printf(" (%s)", name);
  else
    Serial.print(" (name unavailable)");

  Serial.println();
}

void printAllServicesAndCharacteristics(BLEClient *pClient)
{
  Serial.println("Listing all services and characteristics...");

  if (pClient == nullptr || pClient->getServices() == nullptr)
  {
    Serial.println("No services available.");
    return;
  }

  for (auto &serviceEntry : *pClient->getServices())
  {
    BLERemoteService *pRemoteService = serviceEntry.second;
    if (pRemoteService == nullptr)
      continue;

    Serial.printf("Service UUID: %s\n", pRemoteService->getUUID().toString().c_str());

    if (pRemoteService->getCharacteristics() == nullptr)
    {
      Serial.println("  No characteristics found for this service.");
      continue;
    }

    for (auto &characteristicEntry : *pRemoteService->getCharacteristics())
    {
      BLERemoteCharacteristic *pCharacteristic = characteristicEntry.second;
      if (pCharacteristic == nullptr)
        continue;

      printCharacteristicInfo(pCharacteristic);
    }
  }
}

// start generally scanning for all BLE devices so they can be logged.
void setupBLEScan()
{
  Serial.println("Scanning BLE devices...");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new ScannerCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

// scan for specific BLE device and connect to it.
void setupScanSpecific()
{
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new FindSpecificCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  BLEDevice::init("");
  setupBLEScan();
  // setupScanSpecific();
}

bool connectToServer(BLEAddress pAddress)
{
  BLEClient *pClient = BLEDevice::createClient();

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  printAllServicesAndCharacteristics(pClient);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(bmeServiceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  someCharacteristic = pRemoteService->getCharacteristic(characteristicUUID);

  if (someCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");

  // Assign callback functions for the Characteristics
  someCharacteristic->registerForNotify(notifyCallback);
  return true;
}

void loop()
{
  // connectLoopForSpecificDevice();
  delay(1000);

  scanForBLEDevices();
  delay(22000);

  //  digitalWrite(LED_BUILTIN, HIGH);
  //   delay(1000);
  //   digitalWrite(LED_BUILTIN, LOW);
  //   delay(300);
}

void connectLoopForSpecificDevice()
{
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress))
    {
      Serial.println("We are now connected to the BLE Server.");
      // Activate the Notify property of each Characteristic
      someCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
      connected = true;
    }
    else
    {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  // // if new temperature readings are available, print in the OLED
  // if (newTemperature && newHumidity)
  // {
  //   newTemperature = false;
  //   newHumidity = false;
  //   printReadings();
  // }
}

void scanForBLEDevices()
{
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
}
