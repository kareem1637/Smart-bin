#ifndef CONSTANTS_H
#define CONSTANTS_H
// Global variables
  int pictureNumber = 0;
  String quantizedPath="";
  String rgb888Path="";
  File rgb888File;
  File quantizedFile;
  camera_fb_t* fb = NULL;

void init_SD() {
    // Initialize SD Card
    if (!SD_MMC.begin()) {
        Serial.println("SD Card Mount Failed");
        return;
    }
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return;
    }
}

void init_EEPROM() {
    // Initialize EEPROM with predefined size
    EEPROM.begin(1);
    EEPROM.write(0, pictureNumber);  // Save the updated picture number to EEPROM
    EEPROM.commit();  // Ensure the changes are written to EEPROM  // Read the last saved picture number from EEPROM
}

void captureAndOpenFiles() {
    // Update file paths with current pictureNumber
    String basePath = "/picture" + String(pictureNumber);
    rgb888Path = basePath + "_888.txt";  // RGB888 file path
    quantizedPath = basePath + "_Quantized.txt";  // Quantized file path

    // Open the files
    fs::FS &fs = SD_MMC;
    rgb888File = fs.open(rgb888Path.c_str(), FILE_WRITE);
    if (!rgb888File) {
        Serial.println("Failed to open RGB888 file for writing");
        return;
    }

    quantizedFile = fs.open(quantizedPath.c_str(), FILE_WRITE);
    if (!quantizedFile) {
        Serial.println("Failed to open quantized file for writing");
        rgb888File.close();  // Close the first file if the second fails
        return;
    }

    // Capture a frame
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        rgb888File.close();   // Close any open files
        quantizedFile.close();
        return;
    }
}

#endif
