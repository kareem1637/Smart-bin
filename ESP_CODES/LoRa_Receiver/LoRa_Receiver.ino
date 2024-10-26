#include <SoftwareSerial.h>
#include <WiFiMulti.h>
#include <WebSocketsServer_Generic.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <esp_sleep.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>  // Include mDNS header

#define _WEBSOCKETS_LOGLEVEL_ 2
#define WS_PORT 8080

// Sleep durations in microseconds
#define SLEEP_12h 43200000000ULL          // 12 hours
#define SLEEP_1h 3600000000ULL            // 1 hour
#define SLEEP_5m 300000000ULL             // 5 minutes
#define SLEEP_1m 60000000ULL              // 1 minute
unsigned long long sleepTime = SLEEP_1m;  //set 1 min as defalut sleeping time

WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);
String incomingString;

// Define pins for LoRa communication
SoftwareSerial lora(22, 23);        //(TX,RX)
bool sleepFlag = false;             // flag to indicate that the time for sleep is received;
const char* ssid = "kareem";        // Replace with your WiFi SSID
const char* password = "16253794";  // Replace with your WiFi password

const char* apiKey = "6752990";              // Replace with your CallMeBot API key
const char* phoneNumber1 = "+201069604282";  // Replace with the phone number

void setup() {
  Serial.begin(9600);
  lora.begin(9600);
  WiFiMulti.addAP(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  // Start mDNS
  if (!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
    return;
  }
  Serial.println("mDNS started. You can now access the device at esp32.local");
  sleepFlag = false;
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


void webSocketEvent(const uint8_t& num, const WStype_t& type, uint8_t* payload, const size_t& length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        webSocket.sendTXT(num, "Connected");
        break;
      }

    case WStype_TEXT:
      {
        String message = String((char*)payload);

        // Parse incoming JSON message
        StaticJsonDocument<200> jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, message);

        if (!error) {
          const char* type = jsonDoc["type"];
          if (strcmp(type, "set_remaining_time") == 0) {

            // Update sleepTime based on received data
            sleepTime =  jsonDoc["remainingTime"];
            sleepFlag = true;
            Serial.printf("Sleep time updated to: %llu microseconds\n", sleepTime);
          } else if (strcmp(type, "heartbeat") == 0) {
            // Handle heartbeat message
            webSocket.sendTXT(num, "Connected");
          }
        } else {
          Serial.println("Error parsing JSON message.");
        }
        break;
      }

    default:
      break;
  }
}


// Function to request the remaining time from the web app
void requestRemainingTime() {
  // Check if the websocket client is connected
  if (webSocket.connectedClients() > 0) {  // Ensure there's at least one client connected
    String jsonMessage;
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["type"] = "request_remaining_time";

    // Serialize JSON document to string
    serializeJson(jsonDoc, jsonMessage);

    // Send the JSON message to all connected clients
    for (uint8_t i = 0; i < webSocket.connectedClients(); i++) {
      webSocket.sendTXT(i, jsonMessage);  // Send to the specific client
    }

    Serial.println("Requested remaining time from web app");
  } else {
    Serial.println("No clients connected to request remaining time.");
  }
}
void loop() {
  webSocket.loop();
  if (lora.available()) {
    incomingString = lora.readString();
    parseAndSend(incomingString);
    requestRemainingTime();
  }
  if (sleepFlag) {
    // Convert sleepTime to a string and send it over LoRa
    String sleepTimeStr = String(sleepTime);
    lora.println("AT+SEND=2," + String(strlen(sleepTimeStr.c_str())) + "," + sleepTimeStr);
    convertMicrosecondsToHoursMinutes(sleepTime);
    goToSleep();
  }
}

void parseAndSend(const String& message) {
  // Find equal sign and comma positions
  int equal_Index = message.indexOf('=');
  int firstComma = message.indexOf(',');
  int secondComma = message.indexOf(',', firstComma + 1);
  int thirdComma = message.indexOf(',', secondComma + 1);

  // Ensure that necessary indices exist
  if (equal_Index != -1 && firstComma != -1 && secondComma != -1 && thirdComma != -1) {
    // Extract message components
    String lengthStr = message.substring(firstComma + 1, secondComma);
    String data = message.substring(secondComma + 1, thirdComma);
    String ID = message.substring(equal_Index + 1, firstComma);
    lengthStr.trim();
    data.trim();
    ID.trim();
    // Proceed only if "Bin Level" exists in the data string
    if (data.indexOf("Bin") != -1) {
      StaticJsonDocument<200> doc;
      populateJsonData(data, doc, ID);

      // Log extracted values
      logExtractedValues(doc, ID);

      // Create the message string for WhatsApp
      String whatsappMessage = createWhatsAppMessage(doc, ID);

      // Send WhatsApp message
      sendWhatsAppMessage(phoneNumber1, whatsappMessage.c_str());

      // Serialize JSON document to string
      String jsonString;
      serializeJson(doc, jsonString);

      // Broadcast JSON string via WebSocket
      webSocket.broadcastTXT(jsonString);

      // Send acknowledgment back to sender via LoRa
      String ack = "ACK";
      lora.println("AT+SEND=2," + String(strlen(ack.c_str())) + "," + ack);
    } else {
      Serial.println("Error: Could not find 'Bin Level' in message.");
    }
  } else {
    Serial.println("Error parsing message.");
  }
}


String BinLocation(String ID) {
  if (ID == "1") {
    return "home";
  } else if (ID == "2") {
    return "office";
  } else if (ID == "3") {
    return "garden";
  } else {
    return "NO BIN found for this ID";
  }
}
void populateJsonData(const String& data, StaticJsonDocument<200>& doc, const String& ID) {
  // Extract bin level
  int binLevelStart = data.indexOf("Bin Level:") + 10;
  String binLevel = data.substring(binLevelStart, data.indexOf('%', binLevelStart));
  binLevel.trim();
  // Extract model prediction and dirtiness
  int modelPredictionStart = data.indexOf("modelPrediction:") + 16;
  String modelPrediction = data.substring(modelPredictionStart, data.indexOf("with", modelPredictionStart));
  modelPrediction.trim();
  int percentageStart = data.indexOf("precntage of") + 12;
  String dirtiness = data.substring(percentageStart, data.indexOf('%', percentageStart));
  dirtiness.trim();
  // Populate the JSON document
  doc["bin level"] = binLevel;
  doc["model prediction"] = modelPrediction.substring(0, modelPrediction.indexOf(' '));
  doc["dirtiness"] = dirtiness + "%";
  doc["bin Location"] = BinLocation(ID);
}

void logExtractedValues(const StaticJsonDocument<200>& doc, const String& ID) {
  Serial.printf("Bin Level: %s, Model Prediction: %s, Dirtiness: %s, Bin ID: %s\n",
                doc["bin level"].as<String>().c_str(),
                doc["model prediction"].as<String>().c_str(),
                doc["dirtiness"].as<String>().c_str(),
                ID.c_str());
}

String createWhatsAppMessage(const StaticJsonDocument<200>& doc, const String& ID) {
  return "Bin Location: " + doc["bin Location"].as<String>() + "\n"
         + "Bin Level: " + doc["bin level"].as<String>() + "%\n"
         + "Bin Surrounding: " + doc["model prediction"].as<String>() + "\n"
         + "Dirtiness: " + doc["dirtiness"].as<String>();
}

// Put ESP32 into deep sleep
void goToSleep() {
  esp_sleep_enable_timer_wakeup(sleepTime);  // Use the updated sleep time
  delay(100);  // Allow time for Serial to flush
  sleepFlag = false;
  esp_deep_sleep_start();
}

void sendWhatsAppMessage(const char* phoneNumber, const char* message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String encodedMessage = urlEncode(message);  // URL encode the message
    String serverPath = String("https://api.callmebot.com/whatsapp.php?phone=") + phoneNumber + "&text=" + encodedMessage + "&apikey=" + apiKey;

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending GET request: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

// Function to convert microseconds to hours and minutes
// Function to convert microseconds to hours, minutes, and seconds
void convertMicrosecondsToHoursMinutesSeconds(unsigned long long microseconds) {
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
