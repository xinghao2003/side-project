#pragma once

#include <Arduino.h>

class Notifier {
public:
  void begin();
  void notifyCapture(const char *path);

private:
  bool wifiReady_ = false;
};
