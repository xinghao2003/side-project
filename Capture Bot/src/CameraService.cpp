#include "CameraService.h"

#include <SD_MMC.h>
#include <esp_camera.h>

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

  snprintf(pathBuffer, pathBufferSize, "/intrusion_%lu.jpg",
           static_cast<unsigned long>(millis()));
  File image = SD_MMC.open(pathBuffer, FILE_WRITE);
  if (!image) {
    esp_camera_fb_return(frame);
    return false;
  }

  const size_t written = image.write(frame->buf, frame->len);
  image.close();
  esp_camera_fb_return(frame);
  return written == frame->len;
}
