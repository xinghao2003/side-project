#include <Arduino.h>

#include "Alarm.h"
#include "CameraTrigger.h"
#include "Config.h"
#include "GsmNotifier.h"
#include "Sensors.h"

enum class SystemState : uint8_t {
  Arming,
  Armed,
  DisarmWindow,
  Alarm,
  Disarmed,
};

SecuritySensors sensors;
AlarmOutput alarmOutput;
GsmNotifier gsmNotifier;
CameraTrigger cameraTrigger;

SystemState state = SystemState::Arming;
unsigned long stateStartedMs = 0;
unsigned long lastSensorReadMs = 0;
unsigned long lastAlarmMs = 0;
unsigned long lastAuthPulseMs = 0;
unsigned long lastAuthFailPulseMs = 0;
unsigned long lastAuthActionMs = 0;
unsigned long lastDiagnosticsMs = 0;
SensorSnapshot latestSnapshot;

void enterState(SystemState next);

void serviceEmergencyOutputs() {
  alarmOutput.update();
  cameraTrigger.update();
}

const __FlashStringHelper *stateName(SystemState current) {
  switch (current) {
  case SystemState::Arming:
    return F("ARMING");
  case SystemState::Armed:
    return F("ARMED");
  case SystemState::DisarmWindow:
    return F("DISARM_WINDOW");
  case SystemState::Alarm:
    return F("ALARM");
  case SystemState::Disarmed:
    return F("DISARMED");
  }

  return F("UNKNOWN");
}

bool authAccepted() {
  const unsigned long now = millis();
  if (digitalRead(Pins::AuthOk) != LOW &&
      digitalRead(Pins::TelegramDisarm) != LOW) {
    return false;
  }

  if (now - lastAuthPulseMs < Timing::AuthPulseDebounceMs) {
    return false;
  }

  lastAuthPulseMs = now;
  return true;
}

bool authFailDetected() {
  const unsigned long now = millis();
  if (digitalRead(Pins::AuthFailAlarm) != LOW) {
    return false;
  }

  if (now - lastAuthFailPulseMs < Timing::AuthFailPulseDebounceMs) {
    return false;
  }

  lastAuthFailPulseMs = now;
  return true;
}

void handleAuthAction() {
  const unsigned long now = millis();
  if (!authAccepted() ||
      now - lastAuthActionMs < Timing::AuthActionCooldownMs) {
    return;
  }

  lastAuthActionMs = now;
  if (state == SystemState::Disarmed) {
    enterState(SystemState::Arming);
  } else {
    enterState(SystemState::Disarmed);
  }
}

void enterState(SystemState next) {
  state = next;
  stateStartedMs = millis();
  digitalWrite(Pins::AuthWindow,
               next == SystemState::DisarmWindow || next == SystemState::Alarm
                   ? HIGH
                   : LOW);

  if (next != SystemState::Alarm) {
    alarmOutput.stop();
  }
}

void triggerEmergency(AlarmReason reason) {
  const unsigned long now = millis();
  if (now - lastAlarmMs < Timing::AlarmCooldownMs && state == SystemState::Alarm) {
    return;
  }

  lastAlarmMs = now;
  enterState(SystemState::Alarm);
  alarmOutput.start(reason);
  cameraTrigger.requestCapture();
  const bool alertSent = gsmNotifier.sendAlert(reason);
  if (Developer::DiagnosticsEnabled) {
    Serial.println(alertSent ? F("main: gsm alert complete")
                             : F("main: gsm alert incomplete"));
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(Pins::AuthOk, INPUT_PULLUP);
  pinMode(Pins::AuthFailAlarm, INPUT_PULLUP);
  pinMode(Pins::TelegramDisarm, INPUT_PULLUP);
  pinMode(Pins::AuthWindow, OUTPUT);
  digitalWrite(Pins::AuthWindow, LOW);
  sensors.begin();
  alarmOutput.begin();
  cameraTrigger.begin();
  gsmNotifier.setServiceCallback(serviceEmergencyOutputs);
  gsmNotifier.begin();
  enterState(SystemState::Arming);
}

void loop() {
  alarmOutput.update();
  cameraTrigger.update();
  handleAuthAction();
  if (authFailDetected()) {
    triggerEmergency(AlarmReason::BreakIn);
  }

  const unsigned long now = millis();
  if (state == SystemState::Disarmed) {
    if (Developer::DiagnosticsEnabled &&
        now - lastDiagnosticsMs >= Timing::DiagnosticsMs) {
      lastDiagnosticsMs = now;
      Serial.println(F("diag state=DISARMED"));
    }
    return;
  }

  if (state == SystemState::Arming &&
      now - stateStartedMs >= Timing::ArmExitDelayMs) {
    enterState(SystemState::Armed);
  }

  if (now - lastSensorReadMs < Timing::SensorPollMs) {
    return;
  }
  lastSensorReadMs = now;

  latestSnapshot = sensors.read();
  if (Developer::DiagnosticsEnabled &&
      now - lastDiagnosticsMs >= Timing::DiagnosticsMs) {
    lastDiagnosticsMs = now;
    Serial.print(F("diag state="));
    Serial.print(stateName(state));
    Serial.print(F(" pir="));
    Serial.print(latestSnapshot.pirMotion ? 1 : 0);
    Serial.print(F(" distance_cm="));
    Serial.print(latestSnapshot.distanceCm);
    Serial.print(F(" human_likely="));
    Serial.print(latestSnapshot.humanLikely ? 1 : 0);
    Serial.println();
  }

  if (state == SystemState::Armed && latestSnapshot.humanLikely) {
    enterState(SystemState::DisarmWindow);
    return;
  }

  if (state == SystemState::DisarmWindow &&
      now - stateStartedMs >= Timing::DisarmWindowMs) {
    triggerEmergency(AlarmReason::Intrusion);
  }
}
