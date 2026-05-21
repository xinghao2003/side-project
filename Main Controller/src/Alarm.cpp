#include "Alarm.h"

#include "Config.h"

void AlarmOutput::begin() {
  pinMode(Pins::Buzzer, OUTPUT);
  digitalWrite(Pins::Buzzer, LOW);
}

void AlarmOutput::start(AlarmReason reason) {
  active_ = true;
  reason_ = reason;
  lastToggleMs_ = 0;
}

void AlarmOutput::stop() {
  active_ = false;
  buzzerOn_ = false;
  digitalWrite(Pins::Buzzer, LOW);
}

void AlarmOutput::update() {
  if (!active_) {
    return;
  }

  const unsigned long now = millis();
  const unsigned long intervalMs = reason_ == AlarmReason::BreakIn ? 150 : 250;
  if (now - lastToggleMs_ < intervalMs) {
    return;
  }

  lastToggleMs_ = now;
  buzzerOn_ = !buzzerOn_;
  digitalWrite(Pins::Buzzer, buzzerOn_ ? HIGH : LOW);
}
