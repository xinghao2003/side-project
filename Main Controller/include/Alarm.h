#pragma once

#include <Arduino.h>

enum class AlarmReason : uint8_t {
  Intrusion,
  GasLeak,
};

class AlarmOutput {
public:
  void begin();
  void start(AlarmReason reason);
  void stop();
  void update();
  bool active() const { return active_; }

private:
  bool active_ = false;
  AlarmReason reason_ = AlarmReason::Intrusion;
  unsigned long lastToggleMs_ = 0;
  bool buzzerOn_ = false;
};
