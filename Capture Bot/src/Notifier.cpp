#include "Notifier.h"

#include <SD_MMC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <time.h>

#include "Config.h"

#if __has_include("WifiSettings.h")
#include "WifiSettings.h"
#else
#define WIFI_NOTIFICATIONS_ENABLED 0
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define TELEGRAM_BOT_TOKEN ""
#define TELEGRAM_CHAT_ID ""
#define TELEGRAM_PHOTO_CAPTION "Security alert: photo captured"
#define TELEGRAM_CERT_VALIDATION_ENABLED 1
#define TELEGRAM_ROOT_CA ""
#define NTP_GMT_OFFSET_SECONDS 28800L
#define NTP_DAYLIGHT_OFFSET_SECONDS 0
#endif

namespace {
constexpr char MultipartBoundary[] = "----capturebotboundary";

void printStatus(const __FlashStringHelper *message) {
  Serial.println(message);
}
} // namespace

void Notifier::begin() {
#if WIFI_NOTIFICATIONS_ENABLED
  wifiReady_ = ensureWifi();
  if (wifiReady_) {
    timeSynced_ = ensureTimeSynced();
  }
#else
  wifiReady_ = false;
  timeSynced_ = false;
#endif
}

bool Notifier::notifyCapture(const char *path) {
#if WIFI_NOTIFICATIONS_ENABLED
  if (sendTelegramPhoto(path)) {
    return true;
  }

  queueRetry(path);
  return false;
#else
  (void)path;
  return false;
#endif
}

void Notifier::updateRetries() {
#if WIFI_NOTIFICATIONS_ENABLED
  const unsigned long now = millis();
  for (uint8_t i = 0; i < NetworkSettings::TelegramRetryQueueSize; ++i) {
    if (retryPaths_[i][0] == '\0' || now < retryAfterMs_[i]) {
      continue;
    }

    if (retryQueuedCapture(i)) {
      retryPaths_[i][0] = '\0';
      retryAttempts_[i] = 0;
      retryAfterMs_[i] = 0;
    }
  }
#endif
}

bool Notifier::ensureWifi() {
#if WIFI_NOTIFICATIONS_ENABLED
  if (WiFi.status() == WL_CONNECTED) {
    wifiReady_ = true;
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startedMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedMs < NetworkSettings::WifiConnectTimeoutMs) {
    delay(250);
  }

  wifiReady_ = WiFi.status() == WL_CONNECTED;
  return wifiReady_;
#else
  wifiReady_ = false;
  return false;
#endif
}

bool Notifier::ensureTimeSynced() {
#if WIFI_NOTIFICATIONS_ENABLED
  if (timeSynced_) {
    return true;
  }

  configTime(NTP_GMT_OFFSET_SECONDS, NTP_DAYLIGHT_OFFSET_SECONDS,
             NetworkSettings::NtpServer1, NetworkSettings::NtpServer2);

  const unsigned long startedMs = millis();
  time_t now = time(nullptr);
  while (now < 1700000000L &&
         millis() - startedMs < NetworkSettings::TimeSyncTimeoutMs) {
    delay(250);
    now = time(nullptr);
  }

  timeSynced_ = now >= 1700000000L;
  if (!timeSynced_) {
    printStatus(F("capture-bot: ntp sync failed, telegram skipped"));
  }
  return timeSynced_;
#else
  return false;
#endif
}

bool Notifier::configureSecureClient(WiFiClientSecure &client) {
#if WIFI_NOTIFICATIONS_ENABLED
  client.setTimeout(NetworkSettings::TelegramResponseTimeoutMs / 1000);

#if TELEGRAM_CERT_VALIDATION_ENABLED
  if (strlen(TELEGRAM_ROOT_CA) == 0) {
    printStatus(F("capture-bot: telegram root ca missing"));
    return false;
  }

  if (!ensureTimeSynced()) {
    return false;
  }

  client.setCACert(TELEGRAM_ROOT_CA);
#else
  client.setInsecure();
#endif

  return true;
#else
  (void)client;
  return false;
#endif
}

bool Notifier::sendTelegramPhoto(const char *path) {
#if WIFI_NOTIFICATIONS_ENABLED
  if (!ensureWifi()) {
    printStatus(F("capture-bot: wifi unavailable, telegram skipped"));
    return false;
  }

  File photo = SD_MMC.open(path, FILE_READ);
  if (!photo) {
    printStatus(F("capture-bot: telegram skipped, photo file missing"));
    return false;
  }

  WiFiClientSecure client;
  if (!configureSecureClient(client)) {
    photo.close();
    return false;
  }

  if (!client.connect(NetworkSettings::TelegramHost,
                      NetworkSettings::TelegramPort)) {
    photo.close();
    printStatus(F("capture-bot: telegram connect failed"));
    return false;
  }

  const String head =
      String("--") + MultipartBoundary + "\r\n" +
      "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
      TELEGRAM_CHAT_ID + "\r\n" + "--" + MultipartBoundary + "\r\n" +
      "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
      TELEGRAM_PHOTO_CAPTION + "\r\n" + "--" + MultipartBoundary + "\r\n" +
      "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n" +
      "Content-Type: image/jpeg\r\n\r\n";
  const String tail = String("\r\n--") + MultipartBoundary + "--\r\n";
  const size_t contentLength = head.length() + photo.size() + tail.length();
  const String requestPath =
      String("/bot") + TELEGRAM_BOT_TOKEN + "/sendPhoto";

  client.print(F("POST "));
  client.print(requestPath);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.println(NetworkSettings::TelegramHost);
  client.println(F("Connection: close"));
  client.print(F("Content-Type: multipart/form-data; boundary="));
  client.println(MultipartBoundary);
  client.print(F("Content-Length: "));
  client.println(contentLength);
  client.println();
  client.print(head);

  uint8_t buffer[1024];
  while (photo.available()) {
    const size_t read = photo.read(buffer, sizeof(buffer));
    client.write(buffer, read);
  }

  client.print(tail);
  photo.close();

  const bool accepted = waitForTelegramResponse(client);
  printStatus(accepted ? F("capture-bot: telegram photo sent")
                       : F("capture-bot: telegram send failed"));
  return accepted;
#else
  (void)path;
  return false;
#endif
}

bool Notifier::waitForTelegramResponse(WiFiClientSecure &client) {
  const unsigned long startedMs = millis();
  String statusLine;

  while (client.connected() &&
         millis() - startedMs < NetworkSettings::TelegramResponseTimeoutMs) {
    if (!client.available()) {
      delay(10);
      continue;
    }

    statusLine = client.readStringUntil('\n');
    statusLine.trim();
    break;
  }

  return statusLine.startsWith("HTTP/1.1 200") ||
         statusLine.startsWith("HTTP/1.0 200");
}

void Notifier::queueRetry(const char *path) {
#if WIFI_NOTIFICATIONS_ENABLED
  for (uint8_t i = 0; i < NetworkSettings::TelegramRetryQueueSize; ++i) {
    if (strcmp(retryPaths_[i], path) == 0) {
      return;
    }
  }

  for (uint8_t i = 0; i < NetworkSettings::TelegramRetryQueueSize; ++i) {
    if (retryPaths_[i][0] != '\0') {
      continue;
    }

    strlcpy(retryPaths_[i], path, sizeof(retryPaths_[i]));
    retryAttempts_[i] = 0;
    retryAfterMs_[i] = millis() + NetworkSettings::TelegramRetryIntervalMs;
    Serial.print(F("capture-bot: queued telegram retry "));
    Serial.println(path);
    return;
  }

  printStatus(F("capture-bot: telegram retry queue full"));
#else
  (void)path;
#endif
}

bool Notifier::retryQueuedCapture(uint8_t index) {
#if WIFI_NOTIFICATIONS_ENABLED
  if (retryAttempts_[index] >= NetworkSettings::TelegramMaxRetries) {
    Serial.print(F("capture-bot: dropping telegram retry "));
    Serial.println(retryPaths_[index]);
    return true;
  }

  ++retryAttempts_[index];
  Serial.print(F("capture-bot: telegram retry "));
  Serial.print(retryAttempts_[index]);
  Serial.print(F(" for "));
  Serial.println(retryPaths_[index]);

  if (sendTelegramPhoto(retryPaths_[index])) {
    return true;
  }

  retryAfterMs_[index] = millis() + NetworkSettings::TelegramRetryIntervalMs;
  return false;
#else
  (void)index;
  return true;
#endif
}
