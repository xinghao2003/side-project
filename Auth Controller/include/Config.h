#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t KeypadRowPins[] = {2, 3, 4, 5};
constexpr uint8_t KeypadColPins[] = {6, 7, 8, A0};

constexpr uint8_t RfidReset = 9;
constexpr uint8_t RfidSlaveSelect = 10;

constexpr uint8_t AuthOk = A1;
constexpr uint8_t AuthWindow = A2;
constexpr uint8_t Buzzer = A3;
constexpr uint8_t StatusLed = A4;
} // namespace Pins

namespace Timing {
constexpr unsigned long StatusBlinkMs = 250;
constexpr unsigned long LockoutMs = 30000;
constexpr unsigned long BuzzerShortMs = 80;
constexpr unsigned long BuzzerLongMs = 220;
constexpr unsigned long AuthPulseMs = 300;
} // namespace Timing

namespace Security {
constexpr uint8_t MaxFailedAttempts = 3;
} // namespace Security

namespace Developer {
constexpr bool LogScannedRfidUid = true;
} // namespace Developer

namespace Secrets {
constexpr char KeypadCode[] = "1234";

constexpr const char *AuthorizedCards[] = {
    "DE AD BE EF",
};
constexpr uint8_t AuthorizedCardCount =
    sizeof(AuthorizedCards) / sizeof(AuthorizedCards[0]);
} // namespace Secrets
