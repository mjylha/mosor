#pragma once

#include "solar_provider.h"

/** Setup */
void displayBegin();

/** Fix getting stuck, etc... */
void displayMaintain();

/** Render display with solar and battery data */
void displayShow(const SolarSnapshot& snapshot);
