#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "solar_provider.h"
#include "victron_device_settings.h"

namespace {
constexpr uint8_t displayWidth = 128;
constexpr uint8_t displayHeight = 64;
constexpr uint8_t displayAddress = 0x3C;
constexpr int i2cSda = 21;
constexpr int i2cScl = 22;
constexpr uint32_t wifiRetryInterval = 10000;
constexpr uint32_t mqttRetryInterval = 5000;
constexpr uint32_t mqttPublishInterval = 5000;

Adafruit_SSD1306 display(displayWidth, displayHeight, &Wire, -1);
bool displayReady = false;
WiFiClient wifiClient;
WiFiClientSecure secureWifiClient;
PubSubClient mqttClient;
String mqttHost;
uint16_t mqttPort = 1883;
String mqttTopic;
bool mqttReady = false;
uint32_t lastWifiAttempt = 0;
uint32_t lastMqttAttempt = 0;
uint32_t lastMqttPublish = 0;

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

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
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

bool mqttEnabled() {
  return strcmp(MQTT_ENABLED, "1") == 0 ||
         strcasecmp(MQTT_ENABLED, "true") == 0;
}

bool mqttTlsEnabled() {
  return strcmp(MQTT_URL_IS_TLS, "1") == 0 ||
         strcasecmp(MQTT_URL_IS_TLS, "true") == 0;
}

bool configureMqtt() {
  if (!mqttEnabled()) {
    return false;
  }

  if (strlen(WIFI_SSID) == 0 || strlen(MQTT_URL) == 0) {
    Serial.println("MQTT enabled but Wi-Fi or MQTT URL is empty.");
    return false;
  }

  mqttHost = MQTT_URL;
  if (mqttHost.startsWith("mqtt://")) {
    mqttHost.remove(0, 7);
  } else if (mqttHost.startsWith("mqtts://")) {
    mqttHost.remove(0, 8);
  }

  const int separator = mqttHost.lastIndexOf(':');
  if (separator > 0 && separator < static_cast<int>(mqttHost.length() - 1)) {
    const long configuredPort = mqttHost.substring(separator + 1).toInt();
    if (configuredPort < 1 || configuredPort > 65535) {
      Serial.println("Invalid MQTT port.");
      return false;
    }
    mqttPort = static_cast<uint16_t>(configuredPort);
    mqttHost.remove(separator);
  } else {
    mqttPort = mqttTlsEnabled() ? 8883 : 1883;
  }

  mqttTopic = MQTT_TOPIC_PREFIX;
  if (!mqttTopic.endsWith("/")) {
    mqttTopic += "/";
  }
  mqttTopic += "solar";

  if (mqttTlsEnabled()) {
    secureWifiClient.setInsecure();
    mqttClient.setClient(secureWifiClient);
  } else {
    mqttClient.setClient(wifiClient);
  }
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttReady = true;
  Serial.printf("MQTT configured (%s, %s).\n",
                mqttTlsEnabled() ? "TLS" : "non-TLS",
                mqttHost.c_str());
  return true;
}

void maintainMqtt() {
  if (!mqttReady) {
    return;
  }

  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    mqttClient.disconnect();
    if (now - lastWifiAttempt >= wifiRetryInterval) {
      lastWifiAttempt = now;
      Serial.println("Connecting to Wi-Fi...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return;
  }

  if (!mqttClient.connected() && now - lastMqttAttempt >= mqttRetryInterval) {
    lastMqttAttempt = now;
    String clientId = "mosor-" +
                      String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
    bool connected;
    if (strlen(MQTT_USER) == 0) {
      connected = mqttClient.connect(clientId.c_str());
    } else {
      connected = mqttClient.connect(
          clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
    }
    if (connected) {
      Serial.println("MQTT connected.");
    } else {
      Serial.printf("MQTT connection failed, state: %d.\n",
                    mqttClient.state());
    }
  }

  mqttClient.loop();
}

void publishSnapshot(const SolarSnapshot& snapshot) {
  if (!mqttReady || !mqttClient.connected() ||
      millis() - lastMqttPublish < mqttPublishInterval) {
    return;
  }

  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"batteryVoltage\":%.2f,\"batteryCurrent\":%.2f,"
           "\"panelPower\":%.2f,\"chargerState\":%u,\"errorCode\":%u,"
           "\"status\":%u,\"updatedAt\":%lu}",
           snapshot.batteryVoltage,
           snapshot.batteryCurrent,
           snapshot.panelPower,
           snapshot.chargerState,
           snapshot.errorCode,
           static_cast<unsigned>(snapshot.status),
           static_cast<unsigned long>(snapshot.updatedAt));
  if (mqttClient.publish(mqttTopic.c_str(), payload, true)) {
    lastMqttPublish = millis();
  } else {
    Serial.println("Failed to publish SolarSnapshot.");
  }
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

  configureMqtt();

  Wire.begin(i2cSda, i2cScl);
  if (!i2cDevicePresent(displayAddress)) {
    Serial.printf("No I2C device acknowledged at OLED address 0x%02X.\n",
                  displayAddress);
  } else {
    displayReady = display.begin(SSD1306_SWITCHCAPVCC, displayAddress);
    if (!displayReady) {
      Serial.println("OLED controller acknowledged, but SSD1306 initialization failed.");
    } else {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.display();
      Serial.println("OLED display initialized.");
    }
  }

  if (!solar.begin()) {
    Serial.println("Failed to initialize solar provider.");
  }

  WiFi.onEvent([](WiFiEvent_t event) {
    Serial.printf("WiFi event: %d, status:%d\n", (int)event, WiFi.status());
  });

}

void loop() {
  solar.update();
  maintainMqtt();

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
    publishSnapshot(snapshot);
  }

  delay(100);
}
