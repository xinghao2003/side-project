#pragma once

#include <Arduino.h>

#include "Alarm.h"

class GsmNotifier {
public:
  void begin();
  void sendAlert(AlarmReason reason);

private:
  void sendCommand(const __FlashStringHelper *command, unsigned long waitMs = 500);
  void sendCommand(const char *command, unsigned long waitMs = 500);
};
