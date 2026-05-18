#include "Auth.h"

#include <SPI.h>
#include <string.h>

#include "Config.h"

namespace {
char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

byte rowPins[] = {Pins::KeypadRowPins[0], Pins::KeypadRowPins[1],
                  Pins::KeypadRowPins[2], Pins::KeypadRowPins[3]};
byte colPins[] = {Pins::KeypadColPins[0], Pins::KeypadColPins[1],
                  Pins::KeypadColPins[2], Pins::KeypadColPins[3]};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);
} // namespace

void AccessControl::begin() {
  SPI.begin();
  rfid_ = MFRC522(Pins::RfidSlaveSelect, Pins::RfidReset);
  rfid_.PCD_Init();
  resetKeypadBuffer();
}

bool AccessControl::checkAuthorized() {
  return checkRfid() || checkKeypad();
}

void AccessControl::resetKeypadBuffer() {
  memset(keypadBuffer_, 0, sizeof(keypadBuffer_));
  keypadLen_ = 0;
}

bool AccessControl::checkRfid() {
  if (!rfid_.PICC_IsNewCardPresent() || !rfid_.PICC_ReadCardSerial()) {
    return false;
  }

  char uid[32] = {};
  formatUid(uid, sizeof(uid));
  rfid_.PICC_HaltA();
  rfid_.PCD_StopCrypto1();
  return isAuthorizedUid(uid);
}

bool AccessControl::checkKeypad() {
  const char key = keypad.getKey();
  if (!key) {
    return false;
  }

  if (key == '*') {
    resetKeypadBuffer();
    return false;
  }

  if (key == '#') {
    const bool authorized = strcmp(keypadBuffer_, Secrets::KeypadCode) == 0;
    resetKeypadBuffer();
    return authorized;
  }

  if (keypadLen_ < sizeof(keypadBuffer_) - 1) {
    keypadBuffer_[keypadLen_++] = key;
    keypadBuffer_[keypadLen_] = '\0';
  }

  return false;
}

bool AccessControl::isAuthorizedUid(const char *uid) const {
  for (uint8_t i = 0; i < Secrets::AuthorizedCardCount; ++i) {
    if (strcmp(uid, Secrets::AuthorizedCards[i]) == 0) {
      return true;
    }
  }

  return false;
}

void AccessControl::formatUid(char *buffer, size_t size) const {
  size_t offset = 0;
  for (byte i = 0; i < rfid_.uid.size && offset + 3 < size; ++i) {
    if (i > 0 && offset + 1 < size) {
      buffer[offset++] = ' ';
    }
    snprintf(buffer + offset, size - offset, "%02X", rfid_.uid.uidByte[i]);
    offset += 2;
  }
}
