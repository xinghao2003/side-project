#include <Arduino.h>

#include "CameraService.h"
#include "Config.h"
#include "Notifier.h"

CameraService cameraService;
Notifier notifier;

bool lastTriggerState = false;
unsigned long lastTriggerMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(Pins::TriggerIn, INPUT_PULLDOWN);
  pinMode(Pins::FlashLed, OUTPUT);
  digitalWrite(Pins::FlashLed, LOW);

  const bool cameraReady = cameraService.begin();
  notifier.begin();

  Serial.println(cameraReady ? F("capture-bot: ready")
                             : F("capture-bot: camera/sd init failed"));
}

void loop() {
  const bool triggerState = digitalRead(Pins::TriggerIn) == HIGH;
  const unsigned long now = millis();

  if (triggerState && !lastTriggerState &&
      now - lastTriggerMs >= CaptureSettings::TriggerDebounceMs) {
    lastTriggerMs = now;
    char path[48] = {};

    digitalWrite(Pins::FlashLed, HIGH);
    const bool captured = cameraService.captureToSd(path, sizeof(path));
    digitalWrite(Pins::FlashLed, LOW);

    if (captured) {
      Serial.print(F("capture-bot: saved "));
      Serial.println(path);
      notifier.notifyCapture(path);
    } else {
      Serial.println(F("capture-bot: capture failed"));
    }
  }

  lastTriggerState = triggerState;
  delay(20);
}
