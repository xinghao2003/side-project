#pragma once

#include <Arduino.h>
#include <Keypad.h>
#include <MFRC522.h>

enum class AuthResult : uint8_t {
  None,
  Authorized,
  Denied,
};

class AccessControl {
public:
  void begin();
  AuthResult check();
  void resetKeypadBuffer();

private:
  AuthResult checkRfid();
  AuthResult checkKeypad();
  bool isAuthorizedUid(const char *uid) const;
  void formatUid(char *buffer, size_t size) const;
  void logRfidUid(const char *uid, bool authorized) const;

  MFRC522 rfid_;
  char keypadBuffer_[9] = {};
  uint8_t keypadLen_ = 0;
};
