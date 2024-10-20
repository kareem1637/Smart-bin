#include <SoftwareSerial.h>
#include <esp_sleep.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Sleep durations in microseconds
#define SLEEP_12h 43200000000ULL  // 12 hours
#define SLEEP_1h 3600000000ULL    // 1 hour
#define SLEEP_5m 300000000ULL     // 5 minutes
#define SLEEP_1m 60000000ULL      // 1 minute

// Set your ESP32 MAC Address
uint8_t espMACAddress[] = { 0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66 };
// ESP-CAM MAC Address
uint8_t espCamMACAddress[] = { 0x50, 0xAE, 0xA5, 0x08, 0x0D, 0x30 };

// Peer info structure for ESP-NOW
esp_now_peer_info_t peerInfo;

// Size of a single packet for ESP-NOW
typedef struct struct_message {
  char data[50];  // Data buffer for messages
} struct_message;

// String to hold the status message
String modelPrediction;
bool predictionReceived = false;
struct_message received_Data;  // Model prediction
struct_message espNowSignal;   // Signal to be sent to get the model prediction or to go to sleep
bool isSent = false;
// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&received_Data, incomingData, sizeof(received_Data));
  modelPrediction = String(received_Data.data);  // Convert char array to String
  predictionReceived = true;

  // Print the received message to Serial for debugging
  Serial.printf("Received message: %s, Status: %s\n", received_Data.data, modelPrediction.c_str());
}

// Callback function for ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Success");
    isSent = true;
  } else {
    Serial.println("Delivery Fail");
    isSent = false;
  }
}

// Initialize ESP-NOW
void init_EspNow() {
  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station

  // Change ESP32 MAC Address
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, espMACAddress);
  if (err != ESP_OK) {
    Serial.println("Error setting MAC address");
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Register peer
  memcpy(peerInfo.peer_addr, espCamMACAddress, sizeof(espCamMACAddress));
  peerInfo.channel = 0;  // Use default channel
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
}

// Constants for ultrasonic sensor pins
const unsigned int tankHeight = 14;  // Tank height in cm
const int trigPinBin1 = 12;          // Ultrasonic trigger pin for bin 1
const int echoPinBin1 = 13;          // Ultrasonic echo pin for bin 1
const int trigPinBin2 = 2;           // Ultrasonic trigger pin for bin 2
const int echoPinBin2 = 15;          // Ultrasonic echo pin for bin 2

// Variables for ultrasonic sensor measurements
long durationBin1, durationBin2;
int distanceBin1, distanceBin2;

// Lora communication
SoftwareSerial lora(22, 23);
String lora_RX_address = "1";  // Lora RX address

void setup() {
  Serial.begin(9600);
  lora.begin(9600);

  // Initialize sensor pins
  pinMode(trigPinBin1, OUTPUT);
  pinMode(echoPinBin1, INPUT);
  pinMode(trigPinBin2, OUTPUT);
  pinMode(echoPinBin2, INPUT);

  // Initialize ESP-NOW
  init_EspNow();
  delay(2000);
  predictionReceived = false;  // Reset prediction received flag
  Serial.println("setup complete");
  sendSignal("Send_Data");
}

void loop() {
  bool ack = false;
  // Wait for acknowledgment
  while (!ack && predictionReceived) {
    String binInfoCommand = getBinINFO();
    String message = "AT+SEND=" + lora_RX_address + "," + String(binInfoCommand.length()) + "," + binInfoCommand + "\r\n";
    lora.print(message);
    Serial.print(message);
    //ack = waitForAck();
  ack=true;
    // If acknowledged, send signal to sleep
    if (ack) {
      sendSignal("Sleep");
      goToSleep();
    }
  }
}

// Calculate tank level percentage based on distance
int getTankLevel(unsigned int distance) {
  if (distance >= tankHeight) return 0;  // Tank is empty (0%)
  if (distance <= 2) return 100;         // Tank is full (100%)

  // Calculate the percentage based on distance from the bottom
  int percentage = (tankHeight - distance) * 100 / tankHeight;
  return percentage;
}

// Get bin information from ultrasonic sensors
String getBinINFO() {
  // Read from bin level-1 ultrasonic sensor
  digitalWrite(trigPinBin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinBin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinBin1, LOW);
  durationBin1 = pulseIn(echoPinBin1, HIGH);
  distanceBin1 = durationBin1 * 0.034 / 2;

  // Read from bin level-2 ultrasonic sensor
  digitalWrite(trigPinBin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinBin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinBin2, LOW);
  durationBin2 = pulseIn(echoPinBin2, HIGH);
  distanceBin2 = durationBin2 * 0.034 / 2;

  // Determine average bin level
  int binLevel1 = getTankLevel(distanceBin1);
  //int binLevel2 = getTankLevel(distanceBin2);
  //int averageBinLevel = (binLevel1 + binLevel2) / 2; //avg for two sensor 
  int averageBinLevel = (binLevel1 );//abg for one sensor
  // Construct binInfoCommandstring
  String binInfoCommand = "Bin Level: " + String(averageBinLevel) + "%" + " modelPrediction: " + modelPrediction;
  return binInfoCommand;
}

// Put ESP32 into deep sleep
void goToSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_1m);  // Wake up after 1 minute

  Serial.println("Going to sleep now");
  delay(100);  // Allow time for Serial to flush
  esp_deep_sleep_start();
}

// Wait for acknowledgment from Lora
bool waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < SLEEP_5m) {  // Wait for 5 minutes for an ACK
    if (lora.available()) {
      String response = lora.readStringUntil('\n');  // Read until newline
      if (response.indexOf("ACK") != -1) {           // Check if "ACK" is present
        Serial.println("ACK received");
        while (lora.available()) lora.read();  // Clear remaining serial data
        return true;                           // ACK received, return false
      }
    }
  }
  return false;  // Timeout, return true if ACK not received
}

// Send signal via ESP-NOW
void sendSignal(String text) {
  strcpy(espNowSignal.data, text.c_str());
  esp_err_t result = esp_now_send(espCamMACAddress, (uint8_t *)&espNowSignal, sizeof(espNowSignal));
  Serial.println("Sending " + text);
  delay(100);
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
  /* If the message fails to send, it indicates that the ESP32-CAM is not yet ready to receive the message. 
   In this case, we will retry after 2 seconds, up to 3 attempts. 
   If the message still fails to send, it suggests that the camera is not functioning as expected. 
   Note that this logic applies only to the Send_Data signal. */

  // Retry logic if sending fails
  int trials = 1;
  while (!isSent && trials <= 3 && !predictionReceived) {
    Serial.printf("Attempt %d: Retrying...\n", trials);
    esp_now_send(espCamMACAddress, (uint8_t *)&espNowSignal, sizeof(espNowSignal));
    delay(5000);  // Wait 5 seconds before retrying
    trials++;
  }
  if (!isSent) {
    modelPrediction = "failed";
    predictionReceived = true;
    Serial.println("Failed to receive prediction after 3 attempts.");
  }
}
