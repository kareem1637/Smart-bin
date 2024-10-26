
////////
#include "common.h"
#include "constants.h"
#include "Camera_PINS.h"
#include "preproccessing.h"
//TF_model  
#include "TF_LITE_MODEL.h"
camera_fb_t* image;
void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Start serial communication and initialize camera
  Serial.begin(115200);
  setCameraPixelFormat(PIXFORMAT_JPEG);
  delay(1000);
  init_EEPROM();

  delay(100);
  init_EspNow();
  delay(100);
  init_SD();
  initializeTensorFlowModel(TF_LITE_MODEL_data);
  sleepFlag=false;
}

void loop() {
  if (Send_pred) {
    // Capture and process the image
    image=captureAndOpenFiles();
    if (image) {
      int8_t* image_data = input->data.int8;  // input is (1, 240, 240, 3) - RGB image, size(240,240)
      if (!preprocess_image(image->buf, image->len, image_data, kMaxImageSize)) {
        Serial.println("Image preprocessing failed!");
        return;
      }
      // Update and save picture number in EEPROM
      savePictureNumber();       // Ensure EEPROM commit
      esp_camera_fb_return(image);
      image = nullptr;  // Reset pointer after use
      // Invoke the interpreter
      if (kTfLiteOk != interpreter->Invoke()) {
        MicroPrintf("Invoke failed.");
      }

      // Get inference results
      output = interpreter->output(0);
      float score = output->data.f[0];  // output is float32
      String pred = "";
      if (score > 0.4) {
        pred = "dirty";
      } else {
        pred = "clean";
      }
      score=score*100;
      pred=pred+" with precntage of "+ String(score)+"%";
      Serial.printf("The area is: %s, and the MODEL score: %f\n", pred.c_str(), score);
      strcpy(Sent_Data.data, pred.c_str());
      // Send message via ESP-NOW
      SendPred();
    }
    // Call cleanup before program exit or deep sleep
    cleanup();
    Send_pred = false;
  }
  if (sleepSignal && sleepFlag) {
    sleepSignal = false;
    sleepFlag=false;
    goToSleep();
  }
  delay(200);  // Small delay to prevent CPU overuse
}
