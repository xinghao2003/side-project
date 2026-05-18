#include "CameraTrigger.h"

#include "Config.h"

void CameraTrigger::begin() {
  pinMode(Pins::CameraTrigger, OUTPUT);
  digitalWrite(Pins::CameraTrigger, LOW);
}

void CameraTrigger::requestCapture() {
  digitalWrite(Pins::CameraTrigger, HIGH);
  pulsing_ = true;
  pulseStartedMs_ = millis();
}

void CameraTrigger::update() {
  if (pulsing_ && millis() - pulseStartedMs_ >= Timing::CameraPulseMs) {
    digitalWrite(Pins::CameraTrigger, LOW);
    pulsing_ = false;
  }
}
