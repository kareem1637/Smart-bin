#include <constants.h>

uint8_t espMACAddress[] = { 0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66 };
uint8_t espCamMACAddress[] = { 0x50, 0xAE, 0xA5, 0x08, 0x0D, 0x30 };
esp_now_peer_info_t peerInfo;

struct_message Sent_Data;
struct_message Signal;
bool sleepSignal = false;
bool Send_pred = false;
int pictureNumber = 0;
String jpegPath = "";
File jpegFile;
camera_fb_t* fb = NULL;
unsigned long long sleepTime = SLEEP_1m;
bool sleepFlag = false;

// Callback function for ESP-NOW
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Success");
  } else {
    Serial.println("Delivery Fail");
  }
}

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&Signal, incomingData, sizeof(Signal));
  if (String(Signal.data) == "Send_Data") {
    Send_pred = true;
    Serial.printf("Received message: %s\n", Signal.data);
  } else if (String(Signal.data) == "Sleep") {
    sleepSignal = true;
    Serial.printf("Received message: %s\n", Signal.data);
  } else {
    String Timer = String(Signal.data);
    char* endPtr;
    sleepTime = strtoull(Timer.c_str(), &endPtr, 10);
    if (endPtr == Timer.c_str()) {
      Serial.println("Conversion failed, using default sleep time.");
      sleepTime = SLEEP_1m;
    }
    Serial.printf("Received sleep time: %llu microseconds\n", sleepTime);
    sleepFlag = true;
  }
}

// Initialize ESP-NOW
void init_EspNow() {
  WiFi.mode(WIFI_STA);
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, &espCamMACAddress[0]);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  memcpy(peerInfo.peer_addr, espMACAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

// Initialize the SD Card
void init_SD() {
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }
  Serial.println("SD Card initialized successfully.");
}

// Initialize EEPROM with a marker to detect first-time use
void init_EEPROM() {
  EEPROM.begin(2);  // Reserve 2 bytes: 1 for marker, 1 for picture count
  
  // Check if marker exists (let's assume 0xFF as the uninitialized state)
  if (EEPROM.read(0) != 0xAA) {  // Check if marker byte is 0xAA (or any unique value)
    Serial.println("First run after uploading code. Resetting picture count to 0.");
    EEPROM.write(0, 0xAA);  // Set marker
    pictureNumber = 0;      // Start count from 0
    EEPROM.write(1, pictureNumber);
    EEPROM.commit();
  } else {
    // Not the first run; read saved picture count
    pictureNumber = EEPROM.read(1);
    Serial.print("Continuing from saved picture number: ");
    Serial.println(pictureNumber);
  }
}

// Capture image and open files
camera_fb_t* captureAndOpenFiles() {
  String basePath = "/picture" + String(pictureNumber);
  jpegPath = basePath + ".jpeg";

  fs::FS& fs = SD_MMC;
  jpegFile = fs.open(jpegPath.c_str(), FILE_WRITE);
  if (!jpegFile) {
    Serial.println("Failed to open jpeg file for writing");
    return nullptr; // Return nullptr on failure
  }

  captureImage();

  if (fb) { // Check if fb is valid before writing
    jpegFile.write(fb->buf, fb->len);  // fb->buf contains the JPEG data, fb->len is the data length
    Serial.printf("Saved file to path: %s\n", jpegPath.c_str());
    jpegFile.close();
    esp_camera_fb_return(fb);
    fb = nullptr;
    delay(200);
    setCameraPixelFormat(PIXFORMAT_RGB565);
    captureImage(); // Capture another image after changing the pixel format
  } else {
    Serial.println("No valid framebuffer to save image.");
    jpegFile.close();
    return nullptr; // Return nullptr if no image was captured
  }

  return fb; // Return the captured frame buffer or nullptr if capture failed
}

// Function to convert microseconds to hours, minutes, and seconds
void printSleepTime(unsigned long long microseconds) {
  int hours, minutes, seconds;

  // Constants for conversion
  const unsigned long long MICROSECONDS_IN_A_SECOND = 1000000;
  const unsigned long long SECONDS_IN_A_MINUTE = 60;
  const unsigned long long MINUTES_IN_AN_HOUR = 60;

  // Calculate total seconds
  unsigned long long totalSeconds = microseconds / MICROSECONDS_IN_A_SECOND;

  // Calculate hours, minutes, and seconds
  hours = totalSeconds / (SECONDS_IN_A_MINUTE * MINUTES_IN_AN_HOUR);
  minutes = (totalSeconds % (SECONDS_IN_A_MINUTE * MINUTES_IN_AN_HOUR)) / SECONDS_IN_A_MINUTE;
  seconds = totalSeconds % SECONDS_IN_A_MINUTE;

  // Print the result
  Serial.printf("Going to sleep now for %d Hours, %d Minutes, and %d Seconds \n", hours, minutes, seconds);
}


void goToSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_1m);
  printSleepTime(sleepTime);
  delay(100);
  esp_deep_sleep_start();
  
}

void SendPred() {
  esp_err_t result = esp_now_send(espMACAddress, (uint8_t*)&Sent_Data, sizeof(Sent_Data));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
}
void captureImage(){
  for (int i = 0; i < 2; i++) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = nullptr;
    }
  }

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Final camera capture failed");
    return;
  } else {
    Serial.println("Image captured successfully.");
  }
}
void savePictureNumber() {
  pictureNumber++;  // Increment picture count
  EEPROM.write(1, pictureNumber);  // Save updated picture count
  EEPROM.commit();
}