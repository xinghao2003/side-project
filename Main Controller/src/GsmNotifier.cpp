#include "GsmNotifier.h"

#include <string.h>

#include "Config.h"

GsmNotifier::GsmNotifier()
    : sim800_(Pins::Sim800Rx, Pins::Sim800Tx) {}

void GsmNotifier::begin() {
  sim800_.begin(9600);
  delay(1000);
  initializeModem();
}

bool GsmNotifier::sendAlert(AlarmReason reason) {
  const bool smsSent = sendSms(reason);
  const bool callPlaced = placeCall();
  return smsSent && callPlaced;
}

void GsmNotifier::setServiceCallback(void (*callback)()) {
  serviceCallback_ = callback;
}

bool GsmNotifier::initializeModem() {
  printDiagnostic(F("gsm: initializing"));
  const bool ready = waitForReady(Timing::GsmRegistrationTimeoutMs);
  const bool echoOff =
      sendCommand(F("ATE0"), "OK", Timing::GsmCommandTimeoutMs);
  const bool textMode =
      sendCommand(F("AT+CMGF=1"), "OK", Timing::GsmCommandTimeoutMs);
  const bool registered =
      sendCommand(F("AT+CREG?"), "+CREG: 0,1", Timing::GsmCommandTimeoutMs) ||
      sendCommand(F("AT+CREG?"), "+CREG: 0,5", Timing::GsmCommandTimeoutMs);

  if (!ready || !echoOff || !textMode || !registered) {
    printDiagnostic(F("gsm: init incomplete"));
    return false;
  }

  printDiagnostic(F("gsm: ready"));
  return true;
}

bool GsmNotifier::waitForReady(unsigned long timeoutMs) {
  const unsigned long startedMs = millis();
  while (millis() - startedMs < timeoutMs) {
    if (sendCommand(F("AT"), "OK", Timing::GsmCommandTimeoutMs)) {
      return true;
    }
    waitWithService(1000);
  }

  return false;
}

bool GsmNotifier::sendSms(AlarmReason reason) {
  printDiagnostic(F("gsm: sending sms"));
  if (!sendCommand(F("AT+CMGF=1"), "OK", Timing::GsmCommandTimeoutMs)) {
    return false;
  }

  sim800_.print(F("AT+CMGS=\""));
  sim800_.print(Secrets::AlertPhoneNumber);
  sim800_.println(F("\""));
  if (!waitForResponse(">", Timing::GsmSmsPromptTimeoutMs)) {
    printDiagnostic(F("gsm: sms prompt timeout"));
    return false;
  }

  sim800_.print(reason == AlarmReason::GasLeak
                    ? F("ALERT: methane gas leak detected")
                    : F("ALERT: confirmed intrusion detected"));
  sim800_.write(26);

  const char *matched = nullptr;
  const bool accepted = waitForAnyResponse("+CMGS:", "ERROR",
                                           Timing::GsmSmsSendTimeoutMs,
                                           &matched);
  if (!accepted || strcmp(matched, "ERROR") == 0) {
    printDiagnostic(F("gsm: sms failed"));
    return false;
  }

  waitForResponse("OK", Timing::GsmCommandTimeoutMs);
  printDiagnostic(F("gsm: sms accepted"));
  return true;
}

bool GsmNotifier::placeCall() {
  printDiagnostic(F("gsm: placing call"));
  sim800_.print(F("ATD"));
  sim800_.print(Secrets::AlertPhoneNumber);
  sim800_.println(F(";"));

  const char *matched = nullptr;
  const bool dialAccepted =
      waitForAnyResponse("OK", "ERROR", Timing::GsmCommandTimeoutMs, &matched);
  if (!dialAccepted || strcmp(matched, "ERROR") == 0) {
    printDiagnostic(F("gsm: call failed"));
    return false;
  }

  waitWithService(Timing::GsmCallDurationMs);
  const bool hungUp =
      sendCommand(F("ATH"), "OK", Timing::GsmCommandTimeoutMs);
  printDiagnostic(hungUp ? F("gsm: call complete") : F("gsm: hangup failed"));
  return hungUp;
}

bool GsmNotifier::sendCommand(const __FlashStringHelper *command,
                              const char *expected,
                              unsigned long timeoutMs) {
  drainInput();
  if (Developer::GsmCommandEcho) {
    Serial.print(F("gsm tx: "));
    Serial.println(command);
  }
  sim800_.println(command);
  return waitForResponse(expected, timeoutMs);
}

bool GsmNotifier::sendCommand(const char *command, const char *expected,
                              unsigned long timeoutMs) {
  drainInput();
  if (Developer::GsmCommandEcho) {
    Serial.print(F("gsm tx: "));
    Serial.println(command);
  }
  sim800_.println(command);
  return waitForResponse(expected, timeoutMs);
}

bool GsmNotifier::waitForResponse(const char *expected,
                                  unsigned long timeoutMs) {
  const char *matched = nullptr;
  return waitForAnyResponse(expected, "ERROR", timeoutMs, &matched) &&
         strcmp(matched, expected) == 0;
}

bool GsmNotifier::waitForAnyResponse(const char *expectedA,
                                     const char *expectedB,
                                     unsigned long timeoutMs,
                                     const char **matched) {
  char response[128] = {};
  uint8_t len = 0;
  const unsigned long startedMs = millis();

  while (millis() - startedMs < timeoutMs) {
    service();
    while (sim800_.available()) {
      const char c = static_cast<char>(sim800_.read());
      if (Developer::GsmCommandEcho) {
        Serial.write(c);
      }

      if (len < sizeof(response) - 1) {
        response[len++] = c;
        response[len] = '\0';
      } else {
        memmove(response, response + 1, sizeof(response) - 2);
        response[sizeof(response) - 2] = c;
        response[sizeof(response) - 1] = '\0';
      }

      if (strstr(response, expectedA) != nullptr) {
        *matched = expectedA;
        return true;
      }
      if (strstr(response, expectedB) != nullptr) {
        *matched = expectedB;
        return true;
      }
    }
  }

  return false;
}

void GsmNotifier::waitWithService(unsigned long durationMs) {
  const unsigned long startedMs = millis();
  while (millis() - startedMs < durationMs) {
    service();
    delay(10);
  }
}

void GsmNotifier::service() {
  if (serviceCallback_ != nullptr) {
    serviceCallback_();
  }
}

void GsmNotifier::drainInput() {
  while (sim800_.available()) {
    sim800_.read();
  }
}

void GsmNotifier::printDiagnostic(
    const __FlashStringHelper *message) const {
  if (Developer::DiagnosticsEnabled) {
    Serial.println(message);
  }
}

void GsmNotifier::printDiagnostic(const char *message) const {
  if (Developer::DiagnosticsEnabled) {
    Serial.println(message);
  }
}
