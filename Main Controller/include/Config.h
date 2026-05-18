#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t Pir = 2;
constexpr uint8_t Buzzer = 3;
constexpr uint8_t UltrasonicTrig = 4;
constexpr uint8_t UltrasonicEcho = 5;
constexpr uint8_t CameraTrigger = 6;
constexpr uint8_t AuthOk = 7;
constexpr uint8_t AuthWindow = 8;

constexpr uint8_t GasAnalog = A0;
} // namespace Pins

namespace Timing {
constexpr unsigned long SensorPollMs = 250;
constexpr unsigned long ArmExitDelayMs = 10000;
constexpr unsigned long DisarmWindowMs = 10000;
constexpr unsigned long AlarmCooldownMs = 30000;
constexpr unsigned long CameraPulseMs = 250;
constexpr unsigned long AuthPulseDebounceMs = 1000;
} // namespace Timing

namespace Thresholds {
constexpr uint16_t GasDangerRaw = 620;
constexpr uint16_t MinHumanDistanceCm = 35;
constexpr uint16_t MaxHumanDistanceCm = 220;
constexpr uint8_t IntrusionConfirmSamples = 3;
} // namespace Thresholds

namespace Secrets {
constexpr const char *AlertPhoneNumber = "+60123456789";
} // namespace Secrets
