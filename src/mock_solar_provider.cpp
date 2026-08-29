#include "mock_solar_provider.h"

#include <math.h>

MockSolarProvider::MockSolarProvider(uint32_t seed)
    : seed_(seed == 0 ? 1 : seed), startedAt_(0), started_(false) {}

bool MockSolarProvider::begin() {
  startedAt_ = millis();
  started_ = true;
  snapshot_ = SolarSnapshot{};
  snapshot_.status = SolarDataStatus::Fresh;
  update();
  return true;
}

void MockSolarProvider::update() {
  if (!started_) {
    return;
  }

  const float phase = (millis() - startedAt_) / 30000.0f;
  const float daylight = max(0.0f, sinf(phase * 2.0f * PI));
  const float panelPower = daylight * 180.0f + noise(4.0f);

  snapshot_.batteryVoltage = 12.4f + daylight * 0.4f + noise(0.03f);
  snapshot_.batteryCurrent = max(0.0f, panelPower / snapshot_.batteryVoltage);
  snapshot_.panelPower = max(0.0f, panelPower);
  snapshot_.chargerState = daylight < 0.01f ? 0 : 3;
  snapshot_.errorCode = 0;
  snapshot_.updatedAt = millis();
  snapshot_.status = SolarDataStatus::Fresh;
}

bool MockSolarProvider::latest(SolarSnapshot& snapshot) const {
  snapshot = snapshot_;
  return started_;
}

uint32_t MockSolarProvider::nextRandom() {
  seed_ = seed_ * 1664525UL + 1013904223UL;
  return seed_;
}

float MockSolarProvider::noise(float amplitude) {
  const float normalized = (nextRandom() / 4294967295.0f) * 2.0f - 1.0f;
  return normalized * amplitude;
}
