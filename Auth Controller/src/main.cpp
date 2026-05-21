#include <Arduino.h>

#include "Auth.h"
#include "Config.h"

AccessControl accessControl;

bool statusLedOn = false;
uint8_t failedAttempts = 0;
unsigned long lockoutUntilMs = 0;
unsigned long lastBlinkMs = 0;

bool lockedOut() {
  return lockoutUntilMs != 0 && millis() < lockoutUntilMs;
}

void beep(uint16_t frequency, unsigned long durationMs) {
  tone(Pins::Buzzer, frequency, durationMs);
  delay(durationMs + 25);
  noTone(Pins::Buzzer);
}

void notifyAuthorized() {
  beep(2200, Timing::BuzzerShortMs);
  beep(2600, Timing::BuzzerShortMs);
}

void notifyDenied() {
  beep(700, Timing::BuzzerLongMs);
}

void notifyLocked() {
  beep(500, Timing::BuzzerShortMs);
  beep(500, Timing::BuzzerShortMs);
  beep(500, Timing::BuzzerShortMs);
}

void sendLowPulse(uint8_t pin, unsigned long lowMs) {
  digitalWrite(pin, LOW);
  delay(lowMs);
  digitalWrite(pin, HIGH);
}

void recordDeniedAttempt() {
  if (lockedOut()) {
    notifyLocked();
    return;
  }

  ++failedAttempts;
  notifyDenied();

  if (failedAttempts >= Security::MaxFailedAttempts) {
    lockoutUntilMs = millis() + Timing::LockoutMs;
    accessControl.resetKeypadBuffer();
    sendLowPulse(Pins::AuthFailAlarm, Timing::AuthFailPulseMs);
    notifyLocked();
  }
}

void recordAuthorizedAttempt() {
  failedAttempts = 0;
  lockoutUntilMs = 0;
  notifyAuthorized();
  sendLowPulse(Pins::AuthOk, Timing::AuthPulseMs);
}

void updateStatusLed() {
  if (lockedOut()) {
    digitalWrite(Pins::StatusLed, HIGH);
    return;
  }

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
  Serial.begin(9600);
  pinMode(Pins::AuthOk, OUTPUT);
  digitalWrite(Pins::AuthOk, HIGH);
  pinMode(Pins::AuthFailAlarm, OUTPUT);
  digitalWrite(Pins::AuthFailAlarm, HIGH);
  pinMode(Pins::AuthWindow, INPUT);
  pinMode(Pins::StatusLed, OUTPUT);
  pinMode(Pins::Buzzer, OUTPUT);
  digitalWrite(Pins::StatusLed, LOW);
  digitalWrite(Pins::Buzzer, LOW);

  accessControl.begin();
}

void loop() {
  updateStatusLed();

  if (lockedOut()) {
    accessControl.resetKeypadBuffer();
    return;
  }

  const AuthResult result = accessControl.check();
  if (result == AuthResult::Authorized) {
    recordAuthorizedAttempt();
  } else if (result == AuthResult::Denied) {
    recordDeniedAttempt();
  }
}
