#pragma once

// This is the example file for your personal device configuration.
// Copy it to `victron_device_settings.local.h` and change the values below.
// That local file takes precedence over the defaults in
// `victron_device_settings.h` and is ignored by git.

#define MOSOR_VICTRON_DEVICE_NAME "MySmartSolar"
#define MOSOR_VICTRON_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"
#define MOSOR_VICTRON_ENCRYPTION_KEY "0123456789abcdef0123456789abcdef"

#define MQTT_ENABLED "1"
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "pwd"
#define MQTT_URL "aaaaa.s1.eu.hivemq.cloud:8883"
#define MQTT_URL_IS_TLS "1"
#define MQTT_TOPIC_PREFIX "mokki1/"
#define MQTT_USER "dev"
#define MQTT_PASSWORD "tbd"