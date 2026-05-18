#include "Notifier.h"

#if __has_include("WifiSettings.h")
#include "WifiSettings.h"
#else
#define WIFI_NOTIFICATIONS_ENABLED 0
#endif

#if WIFI_NOTIFICATIONS_ENABLED
#include <WiFi.h>
#endif

void Notifier::begin() {
#if WIFI_NOTIFICATIONS_ENABLED
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const unsigned long startedMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedMs < 8000) {
    delay(250);
  }
  wifiReady_ = WiFi.status() == WL_CONNECTED;
#else
  wifiReady_ = false;
#endif
}

void Notifier::notifyCapture(const char *path) {
  (void)path;
  if (!wifiReady_) {
    return;
  }

  // Hook Telegram upload here once the offline capture path is validated.
}
