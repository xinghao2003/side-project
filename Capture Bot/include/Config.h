#pragma once

#include <Arduino.h>
#include <esp_camera.h>

namespace Pins {
constexpr uint8_t TriggerIn = 13;
constexpr uint8_t FlashLed = 4;
} // namespace Pins

namespace CaptureSettings {
constexpr unsigned long TriggerDebounceMs = 750;
constexpr framesize_t FrameSize = FRAMESIZE_SVGA;
constexpr uint8_t JpegQuality = 12;
constexpr uint8_t FrameBuffers = 1;
} // namespace CaptureSettings

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
