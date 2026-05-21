#include "Notifier.h"

#include <SD_MMC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <time.h>

#include "CameraService.h"
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
#define TELEGRAM_ALLOWED_USER_ID ""
#define NTP_GMT_OFFSET_SECONDS 28800L
#define NTP_DAYLIGHT_OFFSET_SECONDS 0
#endif

namespace {
constexpr char MultipartBoundary[] = "----capturebotboundary";

void printStatus(const __FlashStringHelper *message) {
  Serial.println(message);
}
} // namespace

void Notifier::begin(CameraService *cameraService) {
  cameraService_ = cameraService;
  pinMode(Pins::DisarmOut, OUTPUT);
  digitalWrite(Pins::DisarmOut, HIGH);

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
  return notifyCaptureToChat(path, TELEGRAM_CHAT_ID);
}

bool Notifier::notifyCaptureToChat(const char *path, const char *chatId) {
#if WIFI_NOTIFICATIONS_ENABLED
  if (sendTelegramPhoto(path, chatId)) {
    return true;
  }

  if (strcmp(chatId, TELEGRAM_CHAT_ID) == 0) {
    queueRetry(path);
  }
  return false;
#else
  (void)path;
  (void)chatId;
  return false;
#endif
}

void Notifier::updateRetries() {
#if WIFI_NOTIFICATIONS_ENABLED
  pollTelegramCommands();

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

bool Notifier::sendTelegramPhoto(const char *path, const char *chatId) {
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
      chatId + "\r\n" + "--" + MultipartBoundary + "\r\n" +
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
  (void)chatId;
  return false;
#endif
}

void Notifier::pollTelegramCommands() {
#if WIFI_NOTIFICATIONS_ENABLED
  if (strlen(TELEGRAM_ALLOWED_USER_ID) == 0 || strlen(TELEGRAM_BOT_TOKEN) == 0) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastTelegramCommandPollMs_ < NetworkSettings::TelegramCommandPollMs) {
    return;
  }
  lastTelegramCommandPollMs_ = now;

  if (!ensureWifi()) {
    return;
  }

  WiFiClientSecure client;
  if (!configureSecureClient(client)) {
    return;
  }

  if (!client.connect(NetworkSettings::TelegramHost,
                      NetworkSettings::TelegramPort)) {
    return;
  }

  const String requestPath = String("/bot") + TELEGRAM_BOT_TOKEN +
                             "/getUpdates?timeout=0&limit=5&offset=" +
                             String(telegramUpdateOffset_);
  client.print(F("GET "));
  client.print(requestPath);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.println(NetworkSettings::TelegramHost);
  client.println(F("Connection: close"));
  client.println();

  const unsigned long startedMs = millis();
  String response;
  while (millis() - startedMs < NetworkSettings::TelegramResponseTimeoutMs) {
    while (client.available()) {
      response += client.readString();
    }

    if (!client.connected()) {
      break;
    }
    delay(10);
  }

  const int bodyStart = response.indexOf("\r\n\r\n");
  if (bodyStart < 0) {
    return;
  }

  const String body = response.substring(bodyStart + 4);
  int cursor = 0;
  while (true) {
    const int updatePos = body.indexOf("\"update_id\":", cursor);
    if (updatePos < 0) {
      break;
    }

    const int nextUpdatePos = body.indexOf("\"update_id\":", updatePos + 1);
    const int segmentEnd = nextUpdatePos >= 0 ? nextUpdatePos : body.length();
    const String updateJson = body.substring(updatePos, segmentEnd);

    const String updateIdText = extractJsonString(updateJson, 0, "\"update_id\":");
    if (updateIdText.length() > 0) {
      const uint32_t updateId = static_cast<uint32_t>(strtoul(updateIdText.c_str(), nullptr, 10));
      if (updateId >= telegramUpdateOffset_) {
        telegramUpdateOffset_ = updateId + 1;
      }
    }

    if (isAuthorizedCommandUser(updateJson)) {
      const String command = extractJsonString(updateJson, 0, "\"text\":\"");
      const String chatId = extractJsonString(updateJson, 0, "\"chat\":{\"id\":");
      if (command.length() > 0 && chatId.length() > 0) {
        handleTelegramCommand(command, chatId);
      }
    }

    cursor = segmentEnd;
  }
#endif
}

void Notifier::handleTelegramCommand(const String &command, const String &chatId) {
#if WIFI_NOTIFICATIONS_ENABLED
  if (command.startsWith("/disarm")) {
    if (triggerDisarmPulse()) {
      Serial.println(F("capture-bot: disarm pulse sent"));
    }
    return;
  }

  if (!command.startsWith("/capture") || cameraService_ == nullptr) {
    return;
  }

  char path[48] = {};
  const bool captured = cameraService_->captureToSd(path, sizeof(path));
  if (!captured) {
    Serial.println(F("capture-bot: command capture failed"));
    return;
  }

  const bool sent = notifyCaptureToChat(path, chatId.c_str());
  SD_MMC.remove(path);
  Serial.println(sent ? F("capture-bot: command capture sent")
                      : F("capture-bot: command capture send failed"));
#else
  (void)command;
  (void)chatId;
#endif
}

bool Notifier::triggerDisarmPulse() {
  digitalWrite(Pins::DisarmOut, LOW);
  delay(CaptureSettings::DisarmPulseMs);
  digitalWrite(Pins::DisarmOut, HIGH);
  return true;
}

bool Notifier::isAuthorizedCommandUser(const String &updateJson) const {
#if WIFI_NOTIFICATIONS_ENABLED
  if (strlen(TELEGRAM_ALLOWED_USER_ID) == 0) {
    return false;
  }

  const String userId =
      extractJsonString(updateJson, 0, "\"from\":{\"id\":");
  return userId.length() > 0 && userId == TELEGRAM_ALLOWED_USER_ID;
#else
  (void)updateJson;
  return false;
#endif
}

String Notifier::extractJsonString(const String &source, int start,
                                   const char *key) const {
  const int keyPos = source.indexOf(key, start);
  if (keyPos < 0) {
    return String();
  }

  const int valueStart = keyPos + strlen(key);
  if (valueStart >= static_cast<int>(source.length())) {
    return String();
  }

  if (key[strlen(key) - 1] == '"') {
    String value;
    bool escaped = false;
    for (int i = valueStart; i < static_cast<int>(source.length()); ++i) {
      const char c = source[i];
      if (escaped) {
        value += c;
        escaped = false;
        continue;
      }
      if (c == '\\') {
        escaped = true;
        continue;
      }
      if (c == '"') {
        break;
      }
      value += c;
    }
    return value;
  }

  int valueEnd = valueStart;
  while (valueEnd < static_cast<int>(source.length()) &&
         (source[valueEnd] == '-' ||
          (source[valueEnd] >= '0' && source[valueEnd] <= '9'))) {
    ++valueEnd;
  }

  if (valueEnd <= valueStart) {
    return String();
  }

  return source.substring(valueStart, valueEnd);
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

  if (sendTelegramPhoto(retryPaths_[index], TELEGRAM_CHAT_ID)) {
    return true;
  }

  retryAfterMs_[index] = millis() + NetworkSettings::TelegramRetryIntervalMs;
  return false;
#else
  (void)index;
  return true;
#endif
}
