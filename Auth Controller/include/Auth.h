#pragma once

#include <Arduino.h>
#include <Keypad.h>
#include <MFRC522.h>

class AccessControl {
public:
  void begin();
  bool checkAuthorized();
  void resetKeypadBuffer();

private:
  bool checkRfid();
  bool checkKeypad();
  bool isAuthorizedUid(const char *uid) const;
  void formatUid(char *buffer, size_t size) const;

  MFRC522 rfid_;
  char keypadBuffer_[9] = {};
  uint8_t keypadLen_ = 0;
};
