#pragma once

#if defined(MOSOR_VICTRON_SETTINGS)
#include MOSOR_VICTRON_SETTINGS
#elif __has_include("victron_device_settings.local.h")
#include "victron_device_settings.local.h"
#else
#define MOSOR_VICTRON_DEVICE_NAME "SmartSolar"
#define MOSOR_VICTRON_MAC_ADDRESS "E8:DB:84:1C:FF:7A"
#define MOSOR_VICTRON_ENCRYPTION_KEY "0123456789abcdef0123456789abcdef"
#endif
