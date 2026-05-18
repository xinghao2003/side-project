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

AuthResult AccessControl::check() {
  const AuthResult rfidResult = checkRfid();
  if (rfidResult != AuthResult::None) {
    return rfidResult;
  }

  return checkKeypad();
}

void AccessControl::resetKeypadBuffer() {
  memset(keypadBuffer_, 0, sizeof(keypadBuffer_));
  keypadLen_ = 0;
}

AuthResult AccessControl::checkRfid() {
  if (!rfid_.PICC_IsNewCardPresent() || !rfid_.PICC_ReadCardSerial()) {
    return AuthResult::None;
  }

  char uid[32] = {};
  formatUid(uid, sizeof(uid));
  const bool authorized = isAuthorizedUid(uid);
  logRfidUid(uid, authorized);
  rfid_.PICC_HaltA();
  rfid_.PCD_StopCrypto1();
  return authorized ? AuthResult::Authorized : AuthResult::Denied;
}

AuthResult AccessControl::checkKeypad() {
  const char key = keypad.getKey();
  if (!key) {
    return AuthResult::None;
  }

  if (key == '*') {
    resetKeypadBuffer();
    return AuthResult::None;
  }

  if (key == '#') {
    const bool authorized = strcmp(keypadBuffer_, Secrets::KeypadCode) == 0;
    resetKeypadBuffer();
    return authorized ? AuthResult::Authorized : AuthResult::Denied;
  }

  if (keypadLen_ < sizeof(keypadBuffer_) - 1) {
    keypadBuffer_[keypadLen_++] = key;
    keypadBuffer_[keypadLen_] = '\0';
  }

  return AuthResult::None;
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

void AccessControl::logRfidUid(const char *uid, bool authorized) const {
  if (!Developer::LogScannedRfidUid) {
    return;
  }

  Serial.print(F("RFID UID: "));
  Serial.print(uid);
  Serial.print(F(" ["));
  Serial.print(authorized ? F("authorized") : F("unknown"));
  Serial.println(F("]"));
}
