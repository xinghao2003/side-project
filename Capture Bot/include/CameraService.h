#pragma once

#include <Arduino.h>

class CameraService {
public:
  bool begin();
  bool captureToSd(char *pathBuffer, size_t pathBufferSize);
};
