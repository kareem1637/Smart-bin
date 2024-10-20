#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "all_ops_resolver.h"
////////
#include <esp_heap_caps.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include <EEPROM.h>            // Read and write from flash memory
#include <preproccessing.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <constants.h>
#include "Camera_PINS.h"
//TF_model
#include "TF_LITE_MODEL.h"

void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Start serial communication and initialize camera
  Serial.begin(115200);
  init_camera();
  delay(1000);
  init_EEPROM();

  delay(100);
  init_EspNow();
  delay(100);
  init_SD();
  initializeTensorFlowModel(TF_LITE_MODEL_data);
}

void loop() {
  if (Send_pred) {
    // Capture and process the image
    captureAndOpenFiles();
    if (fb) {
      int8_t* image_data = input->data.int8;  // input is (1, 96, 96, 3) - RGB image, size(96,96)
      if (!preprocess_image(fb->buf, fb->len, rgb888File, image_data, kMaxImageSize)) {
        Serial.println("Image preprocessing failed!");
        return;
      }
      // Update and save picture number in EEPROM
      pictureNumber++;
      EEPROM.write(0, pictureNumber);  // Save updated picture number
      EEPROM.commit();                 // Ensure EEPROM commit
      rgb888File.close();
      esp_camera_fb_return(fb);
      fb = nullptr;  // Reset pointer after use
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
  if (sleepSignal) {
    sleepSignal = false;
    goToSleep();
  }
  delay(200);  // Small delay to prevent CPU overuse
}
