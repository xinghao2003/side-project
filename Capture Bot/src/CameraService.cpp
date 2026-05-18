#include "CameraService.h"

#include <SD_MMC.h>
#include <esp_camera.h>
#include <stdlib.h>
#include <string.h>

#include "Config.h"

bool CameraService::begin() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CameraPins::Y2;
  config.pin_d1 = CameraPins::Y3;
  config.pin_d2 = CameraPins::Y4;
  config.pin_d3 = CameraPins::Y5;
  config.pin_d4 = CameraPins::Y6;
  config.pin_d5 = CameraPins::Y7;
  config.pin_d6 = CameraPins::Y8;
  config.pin_d7 = CameraPins::Y9;
  config.pin_xclk = CameraPins::Xclk;
  config.pin_pclk = CameraPins::Pclk;
  config.pin_vsync = CameraPins::Vsync;
  config.pin_href = CameraPins::Href;
  config.pin_sccb_sda = CameraPins::Siod;
  config.pin_sccb_scl = CameraPins::Sioc;
  config.pin_pwdn = CameraPins::Pwdn;
  config.pin_reset = CameraPins::Reset;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = CaptureSettings::FrameSize;
  config.jpeg_quality = CaptureSettings::JpegQuality;
  config.fb_count = CaptureSettings::FrameBuffers;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (esp_camera_init(&config) != ESP_OK) {
    return false;
  }

  return SD_MMC.begin("/sdcard", true);
}

bool CameraService::captureToSd(char *pathBuffer, size_t pathBufferSize) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    return false;
  }

  if (!ensureStorageFor(frame->len)) {
    esp_camera_fb_return(frame);
    return false;
  }

  snprintf(pathBuffer, pathBufferSize, "/intrusion_%lu.jpg",
           static_cast<unsigned long>(millis()));
  File image;
  for (uint8_t attempt = 0; attempt <= CaptureSettings::MaxStorageRotationDeletes;
       ++attempt) {
    image = SD_MMC.open(pathBuffer, FILE_WRITE);
    if (image) {
      break;
    }

    if (!rotateOldestCapture()) {
      esp_camera_fb_return(frame);
      return false;
    }
  }

  const size_t written = image.write(frame->buf, frame->len);
  image.close();
  esp_camera_fb_return(frame);
  if (written == frame->len) {
    return true;
  }

  SD_MMC.remove(pathBuffer);
  return false;
}

bool CameraService::ensureStorageFor(size_t captureBytes) {
  for (uint8_t attempt = 0; attempt <= CaptureSettings::MaxStorageRotationDeletes;
       ++attempt) {
    const uint64_t totalBytes = SD_MMC.totalBytes();
    const uint64_t usedBytes = SD_MMC.usedBytes();
    if (totalBytes == 0 || usedBytes > totalBytes) {
      return false;
    }

    const uint64_t freeBytes = totalBytes - usedBytes;
    if (freeBytes >=
        captureBytes + CaptureSettings::MinFreeBytesAfterCapture) {
      return true;
    }

    if (!rotateOldestCapture()) {
      return false;
    }
  }

  return false;
}

bool CameraService::rotateOldestCapture() {
  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    return false;
  }

  char oldestPath[48] = {};
  unsigned long oldestTimestamp = 0xFFFFFFFFUL;
  File entry = root.openNextFile();
  while (entry) {
    const char *name = entry.name();
    if (!entry.isDirectory() && isCaptureFile(name)) {
      const char *timestampStart = strstr(name, "intrusion_") + strlen("intrusion_");
      const unsigned long timestamp =
          strtoul(timestampStart, nullptr, 10);
      if (timestamp < oldestTimestamp) {
        oldestTimestamp = timestamp;
        strlcpy(oldestPath, name, sizeof(oldestPath));
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();

  if (oldestPath[0] == '\0') {
    return false;
  }

  Serial.print(F("capture-bot: rotating old image "));
  Serial.println(oldestPath);
  return SD_MMC.remove(oldestPath);
}

bool CameraService::isCaptureFile(const char *name) const {
  constexpr char prefix[] = "intrusion_";
  constexpr char suffix[] = ".jpg";
  const char *baseName = name[0] == '/' ? name + 1 : name;
  const size_t nameLen = strlen(name);
  const size_t prefixLen = strlen(prefix);
  const size_t suffixLen = strlen(suffix);
  return nameLen > prefixLen + suffixLen &&
         strncmp(baseName, prefix, prefixLen) == 0 &&
         strcmp(name + nameLen - suffixLen, suffix) == 0;
}
