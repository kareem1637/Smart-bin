
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

camera_fb_t *fb=NULL;
void setup() {
  Serial.begin(115200);
  init_camera();
}

void loop() {
  // Wait for a request (e.g., via serial input)
  if (Serial.available() > 0) {
    char request = Serial.read();

    if (request == 'C') { // Send 'C' to capture image
      capture_and_send_image();
    }
  }
}

void capture_and_send_image() {
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  size_t len = fb->len; // Length of the captured image
  uint8_t *buf = fb->buf; // Buffer containing the captured image

  // Convert RGB565 to RGB888 and send to PC
  for (size_t i = 0; i < len; i += 2) {
    uint16_t rgb565 = buf[i + 1] | (buf[i] << 8);
    uint8_t r = (rgb565 >> 11) & 0x1F;  // Red component
    uint8_t g = (rgb565 >> 5) & 0x3F;   // Green component
    uint8_t b = rgb565 & 0x1F;          // Blue component

    // Convert to 8 bits
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    // Send RGB values to the PC (you can modify this part to fit your needs)
    Serial.write(r);
    Serial.write(g);
    Serial.write(b);
  }

  esp_camera_fb_return(fb); // Return the frame buffer back to the driver
  fb=NULL;
}
