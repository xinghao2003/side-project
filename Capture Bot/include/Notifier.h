#pragma once

#include <Arduino.h>

#include "Config.h"

class CameraService;
class WiFiClientSecure;

class Notifier {
public:
  void begin(CameraService *cameraService);
  bool notifyCapture(const char *path);
  void updateRetries();
  bool notifyCaptureToChat(const char *path, const char *chatId);

private:
  bool ensureWifi();
  bool ensureTimeSynced();
  bool configureSecureClient(WiFiClientSecure &client);
  bool sendTelegramPhoto(const char *path, const char *chatId);
  void pollTelegramCommands();
  void handleTelegramCommand(const String &command, const String &chatId);
  bool triggerDisarmPulse();
  bool isAuthorizedCommandUser(const String &updateJson) const;
  String extractJsonString(const String &source, int start, const char *key) const;
  bool waitForTelegramResponse(WiFiClientSecure &client);
  void queueRetry(const char *path);
  bool retryQueuedCapture(uint8_t index);

  CameraService *cameraService_ = nullptr;
  bool wifiReady_ = false;
  bool timeSynced_ = false;
  unsigned long lastTelegramCommandPollMs_ = 0;
  uint32_t telegramUpdateOffset_ = 0;
  char retryPaths_[NetworkSettings::TelegramRetryQueueSize][48] = {};
  uint8_t retryAttempts_[NetworkSettings::TelegramRetryQueueSize] = {};
  unsigned long retryAfterMs_[NetworkSettings::TelegramRetryQueueSize] = {};
};
