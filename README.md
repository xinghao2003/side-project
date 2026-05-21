# Offline Security Prototype

Three-board PlatformIO prototype for a low-cost security and safety system.

- `Main Controller/`: Arduino Uno R3. Owns sensors, alarm decisions, buzzer, GSM alerts, auth signal intake, and the trigger line to the camera board.
- `Auth Controller/`: Arduino Uno R3. Owns the keypad and MFRC522 RFID reader, then sends an authorization pulse to the main controller.
- `Capture Bot/`: ESP32-CAM. Captures JPEG evidence to MicroSD when triggered. Optional Wi-Fi notification is isolated so the offline path still works.

## Architecture

```text
PIR + HC-SR04
                  |
                  v
        Arduino Uno security state machine
                  |
       +----------+----------+----------------+
       |                     |                |
   SIM800L SMS/call     ESP32-CAM trigger  AUTH_WINDOW
   Buzzer alarm              |                |
                             v                v
                    JPEG saved to MicroSD  Auth Controller
                                             |
                                      AUTH_OK pulse
```

Intrusion requires both PIR motion and a human-sized ultrasonic distance for
repeated samples. After a confirmed intrusion signal, the user has 10 seconds to
disarm. The auth controller checks RFID/keypad input and sends an active-low
`AUTH_OK` pulse before the buzzer, GSM alert/call, and camera trigger run.
Repeated failed auth attempts send an active-low `AUTH_FAIL_ALARM` pulse to
trigger immediate break-in alarm handling. The main controller can be disarmed
by valid auth pulse or by a Telegram-triggered disarm pulse from ESP32-CAM.
When disarmed, another valid auth pulse starts the normal arming delay again.

## Projects

Open each PlatformIO project folder independently:

```powershell
pio run -d ".\Main Controller"
pio run -d ".\Auth Controller"
pio run -d ".\Capture Bot"
```

If `pio` is not on `PATH`, use the PlatformIO virtual environment directly:

```powershell
& $env:USERPROFILE\.platformio\penv\Scripts\pio run -d ".\Main Controller"
& $env:USERPROFILE\.platformio\penv\Scripts\pio run -d ".\Auth Controller"
& $env:USERPROFILE\.platformio\penv\Scripts\pio run -d ".\Capture Bot"
```

## Build, Flash, Monitor

### Main Controller

Build:

```powershell
pio run -d ".\Main Controller"
```

Flash:

```powershell
pio run -d ".\Main Controller" -t upload
```

Monitor USB diagnostics:

```powershell
pio device monitor -d ".\Main Controller" -b 9600
```

Main Controller uses USB Serial at `9600` baud for diagnostics. SIM800L is on
`SoftwareSerial` pins `D9/D10`, so it should not interfere with upload or serial
monitor.

### Auth Controller

Build:

```powershell
pio run -d ".\Auth Controller"
```

Flash:

```powershell
pio run -d ".\Auth Controller" -t upload
```

Monitor RFID UID logs:

```powershell
pio device monitor -d ".\Auth Controller" -b 9600
```

Use this monitor to scan RFID cards and copy the printed UID into
`Auth Controller/include/Config.h`.

### Capture Bot

Build:

```powershell
pio run -d ".\Capture Bot"
```

Flash:

```powershell
pio run -d ".\Capture Bot" -t upload
```

Monitor capture and Telegram logs:

```powershell
pio device monitor -d ".\Capture Bot" -b 115200
```

For many ESP32-CAM programmer boards, flashing requires `GPIO0` connected to
`GND` during reset/upload. Remove that connection and reset again to run normal
firmware.

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
| SIM800L RX from Uno TX | D9 |
| SIM800L TX to Uno RX | D10 |
| Auth controller `AUTH_FAIL_ALARM` input | A0 |
| ESP32-CAM Telegram disarm input | A1 |
| USB serial diagnostics | USB / D0-D1 |

`AUTH_OK` is active-low. Main controller uses `INPUT_PULLUP`; the auth board
idles HIGH and pulls the line LOW for a short authorized pulse.

SIM800L is handled through `SoftwareSerial` so USB Serial remains available for
development diagnostics. Wire from the Arduino perspective:

```text
Main D10 RX <- SIM800L TX
Main D9  TX -> SIM800L RX
GND shared
```

Use a divider or level shifter from Main `D9` into SIM800L RX because the Uno is
5 V and SIM800L logic is lower voltage.

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
| Buzzer | A3 |
| External status LED | A4 |
| Main controller `AUTH_FAIL_ALARM` output | A5 |

Share ground between both Unos. `AUTH_WINDOW` is optional for decision logic; it
is currently used to blink the auth board status LED during the disarm/alarm
window.

The auth signal is a simple active-low pulse. The auth board idles HIGH and
pulls `AUTH_OK` LOW briefly after a valid RFID card or keypad code.

When failed attempts reach lockout, the auth board also emits a short active-low
`AUTH_FAIL_ALARM` pulse to the main board.

Main Controller interprets this pulse by state:

```text
Armed / DisarmWindow / Alarm -> Disarmed
Disarmed -> Arming
Arming -> Disarmed
```

A cooldown prevents rapid repeated arm/disarm changes from one held or repeated
auth pulse.

The auth board locks out after the configured number of failed RFID/keypad
attempts. During lockout it ignores new credentials, holds the status LED on,
uses the buzzer for lockout feedback, and sends an `AUTH_FAIL_ALARM` pulse to
the main board. Wrong keypad submit or unknown RFID card also produce a buzzer
warning.

### ESP32-CAM

| Signal | Pin |
| --- | --- |
| Trigger from Uno D6 | GPIO13 |
| Disarm pulse output to Main A1 | GPIO12 |
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
- Auth keypad code, card UIDs, lockout timing, buzzer timing, and auth pulse:
  `Auth Controller/include/Config.h`
- ESP32-CAM capture settings: `Capture Bot/include/Config.h`
- Optional Wi-Fi settings: copy `Capture Bot/include/WifiSettings.example.h` to
  `Capture Bot/include/WifiSettings.h` and enable `WIFI_NOTIFICATIONS_ENABLED`.

## RFID Setup

The auth controller logs scanned card UIDs to Serial at `9600` baud when
`Developer::LogScannedRfidUid` is enabled in
`Auth Controller/include/Config.h`.

1. Upload `Auth Controller`.
2. Open the serial monitor at `9600` baud.
3. Scan each RFID card.
4. Copy the logged UID into `Secrets::AuthorizedCards`.

Example serial output:

```text
RFID UID: 04 A1 B2 C3 D4 55 80 [unknown]
```

Example config:

```cpp
constexpr const char *AuthorizedCards[] = {
    "04 A1 B2 C3 D4 55 80",
    "13 7F 29 0A",
};
```

## Main Controller Diagnostics

Main Controller uses USB Serial at `9600` baud for development diagnostics.
SIM800L is on `SoftwareSerial`, so diagnostic output is not mixed into the modem
AT command channel.

Typical diagnostics:

```text
diag state=ARMED pir=1 distance_cm=82 human_likely=1
gsm: sending sms
gsm: placing call
```

GSM handling now waits for modem responses such as `OK`, `ERROR`, `+CMGS:`, and
network registration before treating modem actions as successful. Emergency
alerts send SMS first, then place a call for the configured call duration before
hanging up.

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
    #define TELEGRAM_ALLOWED_USER_ID "123456789"
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

When `TELEGRAM_ALLOWED_USER_ID` is configured, the bot also polls for commands
from that user:

- `/disarm` sends a short disarm pulse from ESP32-CAM to the main controller.
- `/capture` captures a fresh photo and replies in Telegram (no persistent
  storage required for command captures).

For development only, `TELEGRAM_CERT_VALIDATION_ENABLED` can be set to `0`; that
uses insecure TLS and should not be used for a deployed system.

## TODO

- Add replay-resistant auth between Main Controller and Auth Controller using
  UART or I2C challenge-response. The current `AUTH_OK` wire is prototype-simple
  and assumes the inter-board wiring is inside a protected enclosure.
- Persist auth lockout state in EEPROM so power-cycling the auth controller does
  not immediately clear failed attempts or an active lockout.

## Electrical Notes

- SIM800L needs a separate stable supply capable of current bursts. Do not power
  it from the Uno 5 V pin.
- Disconnecting SIM800L during upload should no longer be necessary because it
  is not wired to Uno `D0/D1`; USB serial remains dedicated to upload and
  diagnostics.
- ESP32-CAM also needs a stable external 5 V supply.
- HC-SR04 echo is 5 V. If connected to ESP32 in future revisions, level shift it.
- The MFRC522 is a 3.3 V device; use proper level handling when driven by a 5 V
  Uno.
