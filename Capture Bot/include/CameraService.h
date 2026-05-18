#pragma once

#include <Arduino.h>

class CameraService {
public:
  bool begin();
  bool captureToSd(char *pathBuffer, size_t pathBufferSize);

private:
  bool ensureStorageFor(size_t captureBytes);
  bool rotateOldestCapture();
  bool isCaptureFile(const char *name) const;
};
