# Offline Security Prototype

Three-board PlatformIO prototype for a low-cost security and safety system.

- `Main Controller/`: Arduino Uno R3. Owns sensors, alarm decisions, buzzer, GSM alerts, auth signal intake, and the trigger line to the camera board.
- `Auth Controller/`: Arduino Uno R3. Owns the keypad and MFRC522 RFID reader, then sends an authorization pulse to the main controller.
- `Capture Bot/`: ESP32-CAM. Captures JPEG evidence to MicroSD when triggered. Optional Wi-Fi notification is isolated so the offline path still works.

## Architecture

```text
PIR + HC-SR04 + MQ-4
                  |
                  v
        Arduino Uno security state machine
                  |
       +----------+----------+----------------+
       |                     |                |
   SIM800L SMS          ESP32-CAM trigger  AUTH_WINDOW
   Buzzer alarm              |                |
                             v                v
                    JPEG saved to MicroSD  Auth Controller
                                             |
                                      AUTH_OK pulse
```

The Uno treats gas as an immediate emergency. Intrusion requires both PIR motion
and a human-sized ultrasonic distance for repeated samples. After a confirmed
intrusion signal, the user has 10 seconds to disarm. The auth controller checks
RFID/keypad input and sends an active-low `AUTH_OK` pulse before the buzzer, GSM
alert, and camera trigger run.

## Projects

Open each PlatformIO project folder independently:

```powershell
pio run -d ".\Main Controller"
pio run -d ".\Auth Controller"
pio run -d ".\Capture Bot"
```

## Current Prototype Pin Plan

### Main Controller Arduino Uno

| Device | Pin |
| --- | --- |
| PIR signal | D2 |
| Buzzer | D3 |
| HC-SR04 trigger | D4 |
| HC-SR04 echo | D5 |
| ESP32-CAM trigger | D6 |
| Auth controller `AUTH_OK` input | D7 |
| Auth controller `AUTH_WINDOW` output | D8 |
| MQ-4 analog out | A0 |
| SIM800L TX/RX | Uno D0/D1 hardware serial |

`AUTH_OK` is active-low. Main controller uses `INPUT_PULLUP`; the auth board
idles HIGH and pulls the line LOW for a short authorized pulse.

### Auth Controller Arduino Uno

| Device | Pin |
| --- | --- |
| 4x4 keypad rows | D2, D3, D4, D5 |
| 4x4 keypad columns | D6, D7, D8, A0 |
| MFRC522 RST | D9 |
| MFRC522 SDA/SS | D10 |
| MFRC522 MOSI/MISO/SCK | D11/D12/D13 |
| Main controller `AUTH_OK` output | A1 |
| Main controller `AUTH_WINDOW` input | A2 |

Share ground between both Unos. `AUTH_WINDOW` is optional for decision logic; it
is currently used to blink the auth board status LED during the disarm/alarm
window.

### ESP32-CAM

| Signal | Pin |
| --- | --- |
| Trigger from Uno D6 | GPIO13 |
| Flash LED | GPIO4 |
| MicroSD | SD_MMC one-bit mode |

Use a common ground between boards. Level shifting is recommended on any 5 V Uno
output that enters the ESP32-CAM.

The capture bot watches for a rising edge on `GPIO13`. On each valid trigger it
captures a JPEG, saves it to MicroSD first, then optionally sends that saved
photo through Telegram if Wi-Fi notifications are enabled.

If the SD card is near full, the capture bot rotates storage by deleting the
oldest `/intrusion_*.jpg` files until there is room for the new image. It only
rotates files created by this firmware naming pattern.

## Configuration

- Main phone number and sensor thresholds: `Main Controller/include/Config.h`
- Auth keypad code and card UIDs: `Auth Controller/include/Config.h`
- ESP32-CAM capture settings: `Capture Bot/include/Config.h`
- Optional Wi-Fi settings: copy `Capture Bot/include/WifiSettings.example.h` to
  `Capture Bot/include/WifiSettings.h` and enable `WIFI_NOTIFICATIONS_ENABLED`.

## Telegram Setup

Telegram is optional. The ESP32-CAM will still save photos to MicroSD when Wi-Fi
is unavailable, credentials are missing, or Telegram fails.

1. In Telegram, message `@BotFather`, create a bot, and copy the bot token.
2. Send one message to your new bot from the Telegram account or group that
   should receive alerts.
3. Get the chat ID. A common quick check is opening this URL in a browser after
   replacing the token:

   ```text
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```

4. Copy `Capture Bot/include/WifiSettings.example.h` to
   `Capture Bot/include/WifiSettings.h`.
5. Set:

   ```cpp
   #define WIFI_NOTIFICATIONS_ENABLED 1
   #define WIFI_SSID "your-wifi"
   #define WIFI_PASSWORD "your-password"
   #define TELEGRAM_BOT_TOKEN "123456:replace-me"
   #define TELEGRAM_CHAT_ID "123456789"
   #define TELEGRAM_PHOTO_CAPTION "Security alert: photo captured"
   #define TELEGRAM_CERT_VALIDATION_ENABLED 1
   #define TELEGRAM_ROOT_CA "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n"
   #define NTP_GMT_OFFSET_SECONDS 28800L
   #define NTP_DAYLIGHT_OFFSET_SECONDS 0
   ```

With certificate validation enabled, the ESP32-CAM syncs time through NTP before
opening the Telegram HTTPS connection. If time sync fails or `TELEGRAM_ROOT_CA`
is empty, Telegram sending is skipped/retried instead of using insecure TLS.

Failed Telegram sends are queued in RAM and retried every configured interval
while the board remains powered. The photo remains on MicroSD either way. The
retry queue is not persisted across reboot.

For development only, `TELEGRAM_CERT_VALIDATION_ENABLED` can be set to `0`; that
uses insecure TLS and should not be used for a deployed system.

## Electrical Notes

- SIM800L needs a separate stable supply capable of current bursts. Do not power
  it from the Uno 5 V pin.
- ESP32-CAM also needs a stable external 5 V supply.
- MQ-4 needs calibration and warm-up before the raw threshold is meaningful.
- HC-SR04 echo is 5 V. If connected to ESP32 in future revisions, level shift it.
- The MFRC522 is a 3.3 V device; use proper level handling when driven by a 5 V
  Uno.
