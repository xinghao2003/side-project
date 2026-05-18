#pragma once

#include <Arduino.h>

class CameraTrigger {
public:
  void begin();
  void requestCapture();
  void update();

private:
  bool pulsing_ = false;
  unsigned long pulseStartedMs_ = 0;
};
