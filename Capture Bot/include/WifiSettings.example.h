#pragma once

// Copy this file to WifiSettings.h when Wi-Fi/Telegram notification is needed.
// The offline prototype still captures to MicroSD when this file is absent or
// WIFI_NOTIFICATIONS_ENABLED is 0.
#define WIFI_NOTIFICATIONS_ENABLED 0
#define WIFI_SSID "your-wifi"
#define WIFI_PASSWORD "your-password"
#define TELEGRAM_BOT_TOKEN "123456:replace-me"
#define TELEGRAM_CHAT_ID "123456789"
#define TELEGRAM_ALLOWED_USER_ID "123456789"
#define TELEGRAM_PHOTO_CAPTION "Security alert: photo captured"

// Keep validation enabled for real deployments. Paste the PEM for the root CA
// that signs api.telegram.org here. If this is enabled and left empty,
// Telegram sending is skipped instead of falling back to insecure TLS.
#define TELEGRAM_CERT_VALIDATION_ENABLED 1
#define TELEGRAM_ROOT_CA ""

// Malaysia is UTC+8 and has no daylight saving time.
#define NTP_GMT_OFFSET_SECONDS 28800L
#define NTP_DAYLIGHT_OFFSET_SECONDS 0
