#include <Arduino.h>

#include "Auth.h"
#include "Config.h"

AccessControl accessControl;

bool pulsingAuth = false;
bool statusLedOn = false;
unsigned long pulseStartedMs = 0;
unsigned long lastBlinkMs = 0;

void startAuthPulse() {
  digitalWrite(Pins::AuthOk, LOW);
  pulsingAuth = true;
  pulseStartedMs = millis();
}

void updateAuthPulse() {
  if (pulsingAuth && millis() - pulseStartedMs >= Timing::AuthPulseMs) {
    digitalWrite(Pins::AuthOk, HIGH);
    pulsingAuth = false;
  }
}

void updateStatusLed() {
  const bool authWindowOpen = digitalRead(Pins::AuthWindow) == HIGH;
  if (!authWindowOpen) {
    statusLedOn = false;
    digitalWrite(Pins::StatusLed, LOW);
    return;
  }

  const unsigned long now = millis();
  if (now - lastBlinkMs < Timing::StatusBlinkMs) {
    return;
  }

  lastBlinkMs = now;
  statusLedOn = !statusLedOn;
  digitalWrite(Pins::StatusLed, statusLedOn ? HIGH : LOW);
}

void setup() {
  pinMode(Pins::AuthOk, OUTPUT);
  digitalWrite(Pins::AuthOk, HIGH);
  pinMode(Pins::AuthWindow, INPUT);
  pinMode(Pins::StatusLed, OUTPUT);
  digitalWrite(Pins::StatusLed, LOW);

  accessControl.begin();
}

void loop() {
  updateAuthPulse();
  updateStatusLed();

  if (!pulsingAuth && accessControl.checkAuthorized()) {
    startAuthPulse();
  }
}
