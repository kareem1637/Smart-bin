#ifndef CONSTANTS_H
#define CONSTANTS_H

// Sleep durations in microseconds
#define SLEEP_12h 43200000000ULL  // 12 hours
#define SLEEP_1m 60000000ULL      // 1 minute

// Camera model
#define CAMERA_MODEL_AI_THINKER

// ESP-NOW parameters
// Set your esp32 MAC Address
uint8_t espMACAddress[] = { 0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66 };
//Esp cam address
uint8_t espCamMACAddress[] = { 0x50, 0xAE, 0xA5, 0x08, 0x0D, 0x30 };
esp_now_peer_info_t peerInfo;  // Peer info structure

// Size of a single packet for ESP-NOW
typedef struct struct_message {
  char data[50];  // Data buffer for messages
} struct_message;

struct_message Sent_Data;  // Model pred data to sent
struct_message Signal;     // Signal to sent the pre or to go to sleep
bool sleepSignal = false;
bool Send_pred = false;
// Global variables
int pictureNumber = 0;      // Current picture number
String rgb888Path = "";     // Path for RGB888 files
File rgb888File;            // File object for RGB888
camera_fb_t* fb = NULL;     // Frame buffer for camera
// Model parameters
constexpr int kNumCols = 240;
constexpr int kNumRows = 240;
constexpr int kNumChannels = 3;
constexpr int kMaxImageSize = kNumCols * kNumRows * kNumChannels;
namespace {
// TFLite objects
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
}
// Memory allocation for the model's intermediate arrays
constexpr int kTensorArenaSize = 1500000;
static uint8_t* tensor_arena = nullptr;
// Initialization functions

// Callback function for ESP-NOW
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Success");
  } else {
    Serial.println("Delivery Fail");
  }
}
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&Signal, incomingData, sizeof(Signal));
  if (String(Signal.data) == "Send_Data") {
    Send_pred = true;
    // Print the received message to Serial for debugging
    Serial.printf("Received message: %s\n", Signal.data);
  } else if (String(Signal.data) == "Sleep") {
    sleepSignal = true;
    // Print the received message to Serial for debugging
    Serial.printf("Received message: %s\n", Signal.data);
  }
}

// Initialize ESP-NOW
void init_EspNow() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // Change ESP32-cam Mac Address
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, &espCamMACAddress[0]);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback to get the status of transmitted packets
  esp_now_register_send_cb(OnDataSent);
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  // Register peer
  memcpy(peerInfo.peer_addr, espMACAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

// Initialize the SD Card
void init_SD() {
  // Attempt to initialize the SD card
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();

  // Check if an SD card is actually attached
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }

  Serial.println("SD Card initialized successfully.");
}

// Initialize EEPROM
void init_EEPROM() {
  // Initialize EEPROM with 1 byte size
  EEPROM.begin(1);

  // Reset pictureNumber during development
  pictureNumber = 0;  // Comment this line when deploying the final version

  // Only write if the picture number has changed to reduce unnecessary writes
  int savedPictureNumber = EEPROM.read(0);
  if (savedPictureNumber != pictureNumber) {
    EEPROM.write(0, pictureNumber);  // Save the updated picture number to EEPROM
    EEPROM.commit();                 // Ensure the changes are written to EEPROM
  }
}
void initializeTensorFlowModel(const unsigned char TF_LITE_MODEL_data[]) {
  // Load model
  model = tflite::GetModel(TF_LITE_MODEL_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
      "Model provided is schema version %d not equal to supported "
      "version %d.",
      model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  // Check available PSRAM and adjust tensor arena size if necessary
  size_t free_psram = ESP.getFreePsram();
  if (free_psram < kTensorArenaSize) {
    // Adjust tensor arena size to fit available PSRAM
    Serial.printf("PSRAM insufficient! Available: %d bytes. Adjusting tensor arena size to fit.\n", free_psram);
    tensor_arena = (uint8_t*)ps_malloc(free_psram - 1024);  // Leave some buffer
  } else {
    tensor_arena = (uint8_t*)ps_malloc(kTensorArenaSize);
  }

  if (tensor_arena == nullptr) {
    Serial.printf("Failed to allocate memory for tensor arena (requested size: %d bytes).\n", kTensorArenaSize);
    return;
  }
  /*include all ops ,You can comment this line and use the other MutableOps 
    but you should know what OPS you are using in ur model and include them , 
    it will save up flash memory*/
  tflite::AllOpsResolver resolver;
  /*static tflite::MicroMutableOpResolver<15> micro_op_resolver; // Adjust size as needed
      micro_op_resolver.AddAveragePool2D();
      micro_op_resolver.AddConv2D();
      micro_op_resolver.AddDepthwiseConv2D();
      micro_op_resolver.AddFullyConnected();
      micro_op_resolver.AddRelu();
      micro_op_resolver.AddRelu6();
      micro_op_resolver.AddReshape();
      micro_op_resolver.AddSoftmax();
      micro_op_resolver.AddMaxPool2D();
      micro_op_resolver.AddResizeBilinear();
      micro_op_resolver.AddMean();
      micro_op_resolver.AddMul();
      micro_op_resolver.AddAdd();
      micro_op_resolver.AddPad(); // Include this if your model uses PAD op
      micro_op_resolver.AddLogistic();*/
  // Build interpreter
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);

  interpreter = &static_interpreter;

  // Allocate memory from tensor_arena for the model's tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get input tensor
  input = interpreter->input(0);
  output = interpreter->output(0);
}
//////////////////////
//helper functions

// Capture image and open files
void captureAndOpenFiles() {
  // Update file paths with current pictureNumber
  String basePath = "/picture" + String(pictureNumber);
  rgb888Path = basePath + "_888.txt";  // RGB888 file path

  // Open the files
  fs::FS& fs = SD_MMC;
  rgb888File = fs.open(rgb888Path.c_str(), FILE_WRITE);
  if (!rgb888File) {
    Serial.println("Failed to open RGB888 file for writing");
    return;
  }

  // Skip the first two images to avoid bad frames
  for (int i = 0; i < 2; i++) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;  // Skip this iteration if the capture fails
    }

    if (fb) {
      esp_camera_fb_return(fb);  // Return the frame buffer to free memory
      fb = NULL;
    }
  }

  // Capture a new image
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Final camera capture failed");
    return;
  } else {
    Serial.println("Image captured successfully.");
  }
}
void goToSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_1m);  // Wake up after a specific time duration (1 hour) multiply this to 12 to get 12 hours SLEEP_DURATION
  Serial.println("Going to sleep now");
  delay(100);  // Allow time for Serial to flush
  esp_deep_sleep_start();
}
// Memory cleanup function
void cleanup() {
  if (tensor_arena != nullptr) {
    free(tensor_arena);      // Free the tensor arena memory
    tensor_arena = nullptr;  // Avoid dangling pointer
  }
}
void SendPred() {
  esp_err_t result = esp_now_send(espMACAddress, (uint8_t*)&Sent_Data, sizeof(Sent_Data));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
}

#endif
