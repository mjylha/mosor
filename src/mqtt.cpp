#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "mqtt.h"
#include "victron_device_settings.h"

namespace {
constexpr uint32_t wifiRetryInterval = 10000;
constexpr uint32_t mqttRetryInterval = 5000;
constexpr uint32_t mqttPublishInterval = 5000;

WiFiClient wifiClient;
WiFiClientSecure secureWifiClient;
PubSubClient mqttClient;
String mqttHost;
uint16_t mqttPort = 1883;
String mqttTopic;
bool mqttReady = false;
uint32_t lastWifiAttempt = 0;
uint32_t lastMqttAttempt = 0;
uint32_t lastPublish = 0;

bool enabled(const char* value) {
  return strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0;
}

bool configure() {
  if (!enabled(MQTT_ENABLED)) {
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
    mqttPort = enabled(MQTT_URL_IS_TLS) ? 8883 : 1883;
  }

  mqttTopic = MQTT_TOPIC_PREFIX;
  if (!mqttTopic.endsWith("/")) {
    mqttTopic += "/";
  }
  mqttTopic += "solar";

  if (enabled(MQTT_URL_IS_TLS)) {
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
                enabled(MQTT_URL_IS_TLS) ? "TLS" : "non-TLS",
                mqttHost.c_str());
  return true;
}
}  // namespace

void mqttBegin() {
  configure();
  WiFi.onEvent([](WiFiEvent_t event) {
    Serial.printf("WiFi event: %d, status:%d\n", (int)event, WiFi.status());
  });
}

void mqttUpdate() {
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

void mqttPublish(const SolarSnapshot& snapshot) {
  if (!mqttReady || !mqttClient.connected() ||
      millis() - lastPublish < mqttPublishInterval) {
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
    lastPublish = millis();
  } else {
    Serial.println("Failed to publish SolarSnapshot.");
  }
}

bool mqttNetworkProblem() {
  if (!enabled(MQTT_ENABLED)) {
    return false;
  }

  return !mqttReady || WiFi.status() != WL_CONNECTED ||
         !mqttClient.connected();
}
