#include "Sensors.h"

#include "Config.h"

void SecuritySensors::begin() {
  pinMode(Pins::Pir, INPUT);
  pinMode(Pins::UltrasonicTrig, OUTPUT);
  pinMode(Pins::UltrasonicEcho, INPUT);
  pinMode(Pins::GasAnalog, INPUT);
}

SensorSnapshot SecuritySensors::read() {
  SensorSnapshot snapshot;
  snapshot.pirMotion = digitalRead(Pins::Pir) == HIGH;
  snapshot.distanceCm = readDistanceCm();
  snapshot.gasRaw = analogRead(Pins::GasAnalog);
  snapshot.gasDanger = snapshot.gasRaw >= Thresholds::GasDangerRaw;

  const bool distanceLooksHuman =
      snapshot.distanceCm >= Thresholds::MinHumanDistanceCm &&
      snapshot.distanceCm <= Thresholds::MaxHumanDistanceCm;

  if (snapshot.pirMotion && distanceLooksHuman) {
    if (humanSamples_ < Thresholds::IntrusionConfirmSamples) {
      ++humanSamples_;
    }
  } else if (humanSamples_ > 0) {
    --humanSamples_;
  }

  snapshot.humanLikely = humanSamples_ >= Thresholds::IntrusionConfirmSamples;
  return snapshot;
}

uint16_t SecuritySensors::readDistanceCm() const {
  digitalWrite(Pins::UltrasonicTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(Pins::UltrasonicTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(Pins::UltrasonicTrig, LOW);

  const unsigned long durationUs =
      pulseIn(Pins::UltrasonicEcho, HIGH, 30000UL);
  if (durationUs == 0) {
    return 0;
  }

  return static_cast<uint16_t>(durationUs / 58UL);
}
