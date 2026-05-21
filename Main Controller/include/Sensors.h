#pragma once

#include <Arduino.h>

struct SensorSnapshot {
  bool pirMotion = false;
  uint16_t distanceCm = 0;
  bool humanLikely = false;
};

class SecuritySensors {
public:
  void begin();
  SensorSnapshot read();

private:
  uint16_t readDistanceCm() const;
  uint8_t humanSamples_ = 0;
};
