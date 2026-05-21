#pragma once

#include <Arduino.h>
#include <esp_camera.h>

namespace Pins {
constexpr uint8_t TriggerIn = 13;
constexpr uint8_t FlashLed = 4;
constexpr uint8_t DisarmOut = 12;
} // namespace Pins

namespace CaptureSettings {
constexpr unsigned long TriggerDebounceMs = 750;
constexpr unsigned long DisarmPulseMs = 300;
constexpr framesize_t FrameSize = FRAMESIZE_SVGA;
constexpr uint8_t JpegQuality = 12;
constexpr uint8_t FrameBuffers = 1;
constexpr uint8_t MaxStorageRotationDeletes = 8;
constexpr uint32_t MinFreeBytesAfterCapture = 128UL * 1024UL;
} // namespace CaptureSettings

namespace NetworkSettings {
constexpr unsigned long WifiConnectTimeoutMs = 10000;
constexpr unsigned long TimeSyncTimeoutMs = 15000;
constexpr unsigned long TelegramResponseTimeoutMs = 15000;
constexpr unsigned long TelegramCommandPollMs = 5000;
constexpr unsigned long TelegramRetryIntervalMs = 60000;
constexpr uint8_t TelegramMaxRetries = 5;
constexpr uint8_t TelegramRetryQueueSize = 8;
constexpr const char *TelegramHost = "api.telegram.org";
constexpr uint16_t TelegramPort = 443;
constexpr const char *NtpServer1 = "pool.ntp.org";
constexpr const char *NtpServer2 = "time.google.com";
} // namespace NetworkSettings

namespace CameraPins {
constexpr int Pwdn = 32;
constexpr int Reset = -1;
constexpr int Xclk = 0;
constexpr int Siod = 26;
constexpr int Sioc = 27;
constexpr int Y9 = 35;
constexpr int Y8 = 34;
constexpr int Y7 = 39;
constexpr int Y6 = 36;
constexpr int Y5 = 21;
constexpr int Y4 = 19;
constexpr int Y3 = 18;
constexpr int Y2 = 5;
constexpr int Vsync = 25;
constexpr int Href = 23;
constexpr int Pclk = 22;
} // namespace CameraPins
