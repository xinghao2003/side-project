#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "Alarm.h"

class GsmNotifier {
public:
  GsmNotifier();
  void begin();
  bool sendAlert(AlarmReason reason);
  void setServiceCallback(void (*callback)());

private:
  bool initializeModem();
  bool waitForReady(unsigned long timeoutMs);
  bool sendSms(AlarmReason reason);
  bool placeCall();
  bool sendCommand(const __FlashStringHelper *command, const char *expected,
                   unsigned long timeoutMs);
  bool sendCommand(const char *command, const char *expected,
                   unsigned long timeoutMs);
  bool waitForResponse(const char *expected, unsigned long timeoutMs);
  bool waitForAnyResponse(const char *expectedA, const char *expectedB,
                          unsigned long timeoutMs, const char **matched);
  void waitWithService(unsigned long durationMs);
  void service();
  void drainInput();
  void printDiagnostic(const __FlashStringHelper *message) const;
  void printDiagnostic(const char *message) const;

  SoftwareSerial sim800_;
  void (*serviceCallback_)() = nullptr;
};
