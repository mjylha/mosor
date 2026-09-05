#pragma once

#include "solar_provider.h"

void mqttBegin();

/** Ensure mqtt connection (including wifi connection) is up */
void mqttUpdate();

/** Publish solar snapshot to MQTT broker */
void mqttPublish(const SolarSnapshot& snapshot);
