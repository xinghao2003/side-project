#pragma once

#include <Arduino.h>

#include "Config.h"

class WiFiClientSecure;

class Notifier {
public:
  void begin();
  bool notifyCapture(const char *path);
  void updateRetries();

private:
  bool ensureWifi();
  bool ensureTimeSynced();
  bool configureSecureClient(WiFiClientSecure &client);
  bool sendTelegramPhoto(const char *path);
  bool waitForTelegramResponse(WiFiClientSecure &client);
  void queueRetry(const char *path);
  bool retryQueuedCapture(uint8_t index);

  bool wifiReady_ = false;
  bool timeSynced_ = false;
  char retryPaths_[NetworkSettings::TelegramRetryQueueSize][48] = {};
  uint8_t retryAttempts_[NetworkSettings::TelegramRetryQueueSize] = {};
  unsigned long retryAfterMs_[NetworkSettings::TelegramRetryQueueSize] = {};
};
