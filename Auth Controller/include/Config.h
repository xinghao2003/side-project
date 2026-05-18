#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t KeypadRowPins[] = {2, 3, 4, 5};
constexpr uint8_t KeypadColPins[] = {6, 7, 8, A0};

constexpr uint8_t RfidReset = 9;
constexpr uint8_t RfidSlaveSelect = 10;

constexpr uint8_t AuthOk = A1;
constexpr uint8_t AuthWindow = A2;
constexpr uint8_t StatusLed = LED_BUILTIN;
} // namespace Pins

namespace Timing {
constexpr unsigned long AuthPulseMs = 300;
constexpr unsigned long StatusBlinkMs = 250;
} // namespace Timing

namespace Secrets {
constexpr char KeypadCode[] = "1234";

constexpr const char *AuthorizedCards[] = {
    "DE AD BE EF",
};
constexpr uint8_t AuthorizedCardCount =
    sizeof(AuthorizedCards) / sizeof(AuthorizedCards[0]);
} // namespace Secrets
