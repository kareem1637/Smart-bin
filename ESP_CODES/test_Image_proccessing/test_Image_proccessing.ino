#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"   
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32        // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // Read and write from flash memory
#include <preproccessing.h>
#define CAMERA_MODEL_AI_THINKER
#include <camera_pins.h>
#include <constants.h>


void setup() {
    Serial.begin(115200);
    init_camera();
    init_SD();
    

}

void loop() {
    // Wait for the 'CAPTURE' command from the serial input
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');  // Read the command
        command.trim();  // Remove any extra whitespace or newline characters



        if (command == "CAPTURE") {
            //Call the function to handle file creation and image capture
            captureAndOpenFiles();
            //preprocess_image 
            preprocess_image(fb->buf,fb->len,rgb888File,quantizedFile);
            esp_camera_fb_return(fb);
            // Update and save picture number
            pictureNumber++;
            EEPROM.write(0, pictureNumber);  // Save the updated picture number to EEPROM
            EEPROM.commit();  // Ensure the changes are written to EEPROM

            Serial.println("Image saved as " + quantizedPath + " and " + rgb888Path);
        } else {
            Serial.println("Unknown command. Please send 'CAPTURE' to take an image.");
        }
    }

    delay(100);  // Short delay to prevent overloading the loop
}
