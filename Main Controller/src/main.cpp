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
AlarmReason pendingReason = AlarmReason::Intrusion;

bool authAccepted() {
  const unsigned long now = millis();
  if (digitalRead(Pins::AuthOk) != LOW) {
    return false;
  }

  if (now - lastAuthPulseMs < Timing::AuthPulseDebounceMs) {
    return false;
  }

  lastAuthPulseMs = now;
  return true;
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

  pendingReason = reason;
  lastAlarmMs = now;
  alarmOutput.start(reason);
  cameraTrigger.requestCapture();
  gsmNotifier.sendAlert(reason);
  enterState(SystemState::Alarm);
}

void setup() {
  pinMode(Pins::AuthOk, INPUT_PULLUP);
  pinMode(Pins::AuthWindow, OUTPUT);
  digitalWrite(Pins::AuthWindow, LOW);
  sensors.begin();
  alarmOutput.begin();
  cameraTrigger.begin();
  gsmNotifier.begin();
  enterState(SystemState::Arming);
}

void loop() {
  alarmOutput.update();
  cameraTrigger.update();

  if (authAccepted()) {
    enterState(SystemState::Disarmed);
  }

  const unsigned long now = millis();
  if (state == SystemState::Disarmed) {
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

  const SensorSnapshot snapshot = sensors.read();
  if (snapshot.gasDanger) {
    triggerEmergency(AlarmReason::GasLeak);
    return;
  }

  if (state == SystemState::Armed && snapshot.humanLikely) {
    enterState(SystemState::DisarmWindow);
    return;
  }

  if (state == SystemState::DisarmWindow &&
      now - stateStartedMs >= Timing::DisarmWindowMs) {
    triggerEmergency(AlarmReason::Intrusion);
  }
}
