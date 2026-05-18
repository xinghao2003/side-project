#include "GsmNotifier.h"

#include "Config.h"

void GsmNotifier::begin() {
  Serial.begin(9600);
  delay(1000);
  sendCommand(F("AT"));
  sendCommand(F("ATE0"));
  sendCommand(F("AT+CMGF=1"));
}

void GsmNotifier::sendAlert(AlarmReason reason) {
  Serial.print(F("AT+CMGS=\""));
  Serial.print(Secrets::AlertPhoneNumber);
  Serial.println(F("\""));
  delay(500);
  Serial.print(reason == AlarmReason::GasLeak ? F("ALERT: methane gas leak detected")
                                              : F("ALERT: confirmed intrusion detected"));
  Serial.write(26);
  delay(3000);
}

void GsmNotifier::sendCommand(const __FlashStringHelper *command,
                              unsigned long waitMs) {
  Serial.println(command);
  delay(waitMs);
}

void GsmNotifier::sendCommand(const char *command, unsigned long waitMs) {
  Serial.println(command);
  delay(waitMs);
}
