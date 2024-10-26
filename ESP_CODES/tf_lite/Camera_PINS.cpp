#include "Camera_PINS.h"
camera_config_t config;
bool cameraInitialized = false;
// Function to switch camera pixel format
void setCameraPixelFormat(pixformat_t format) {
  if (cameraInitialized) {
    esp_camera_deinit();  // Deinitialize the camera
    cameraInitialized=false;
    }
    config.pixel_format = format;  // Set the new pixel format
    init_camera();
    

}

// Function to initialize the camera
void init_camera() {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.frame_size = FRAMESIZE_240X240; // Set to 240x240 resolution
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // // Initialize with JPEG format by default; modify as needed
  // setCameraPixelFormat(PIXFORMAT_JPEG);

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("Camera initialized successfully.");
    cameraInitialized=true;
  }
}
