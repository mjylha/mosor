#pragma once

// This file defines the default Victron configuration used by the project.
// If you want to override any of these values for your own device, copy the
// example header to `victron_device_settings.local.h` and set your own values
// there. The local file is preferred automatically and is git-ignored.
//
// Precedence order:
//   1. MOSOR_VICTRON_SETTINGS compile definition
//   2. include/victron_device_settings.local.h
//   3. built-in default values below

#if defined(MOSOR_VICTRON_SETTINGS)
#include MOSOR_VICTRON_SETTINGS
#elif __has_include("victron_device_settings.local.h")
#include "victron_device_settings.local.h"
#else
#define MOSOR_VICTRON_DEVICE_NAME "SmartSolar"
#define MOSOR_VICTRON_MAC_ADDRESS "E8:DB:84:1C:FF:7A"
#define MOSOR_VICTRON_ENCRYPTION_KEY "0123456789abcdef0123456789abcdef"
#define MQTT_ENABLED "0"
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_URL ""
#define MQTT_URL_IS_TLS "1"
#define MQTT_TOPIC_PREFIX "mokki1/"
#define MQTT_USER ""
#define MQTT_PASSWORD ""


#endif
