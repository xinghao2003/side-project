#pragma once

#include <Arduino.h>

struct SensorSnapshot {
  bool pirMotion = false;
  uint16_t distanceCm = 0;
  uint16_t gasRaw = 0;
  bool humanLikely = false;
  bool gasDanger = false;
};

class SecuritySensors {
public:
  void begin();
  SensorSnapshot read();

private:
  uint16_t readDistanceCm() const;
  uint8_t humanSamples_ = 0;
};
