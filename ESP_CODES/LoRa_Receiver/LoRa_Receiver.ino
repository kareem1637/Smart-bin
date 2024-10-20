#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer_Generic.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <esp_sleep.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>  // Include mDNS header

#define _WEBSOCKETS_LOGLEVEL_ 2
#define WS_PORT 8080

// Sleep durations in microseconds
#define SLEEP_12h 43200000000ULL  // 12 hours
#define SLEEP_1h 3600000000ULL    // 1 hour
#define SLEEP_5m 300000000ULL     // 5 minutes
#define SLEEP_1m 60000000ULL      // 1 minute

WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);
String incomingString;

// Define pins for LoRa communication
SoftwareSerial lora(22, 23);  //(TX,RX)

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

  Serial.println();
  Serial.print("WebSocket Server started @ IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(", port: ");
  Serial.println(WS_PORT);

  // Start mDNS
  if (!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
    return;
  }
  Serial.println("mDNS started. You can now access the device at esp32.local");

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
      //Serial.printf("[%u] get Text: %s\n", num, payload);
      webSocket.sendTXT(num, "Connected");
      break;

    default:
      break;
  }
}

void loop() {
  webSocket.loop();
  while (lora.available()) {
    incomingString = lora.readString();
    // Parse the incoming string
    parseAndSend(incomingString);
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
      // Create a JSON document for WebSocket and WhatsApp message
      StaticJsonDocument<200> doc;

      // Extract bin level
      int binLevelStart = data.indexOf("Bin Level:") + 10;
      int binLevelEnd = data.indexOf('%', binLevelStart);
      String binLevel = data.substring(binLevelStart, binLevelEnd);
      binLevel.trim();
      // Extract model prediction (clean/dirty) and dirtiness percentage
      int modelPredictionStart = data.indexOf("modelPrediction:") + 16;
      int modelPredictionEnd = data.indexOf("with", modelPredictionStart);
      String modelPrediction = data.substring(modelPredictionStart, modelPredictionEnd);
       modelPrediction.trim();
      int percentageStart = data.indexOf("precntage of") + 12;
      int percentageEnd = data.indexOf('%', percentageStart);
      String dirtiness = data.substring(percentageStart, percentageEnd);
      dirtiness.trim();
      // Extract model prediction status (clean/dirty)
      int space_index = modelPrediction.indexOf(' ');
      String status = modelPrediction.substring(0, space_index);

      // Retrieve bin location based on the bin ID
      String Location = BinLocation(ID);

      // Populate the JSON document
      doc["bin level"] = binLevel;
      doc["model prediction"] = status;
      doc["dirtiness"] = dirtiness + "%";
      doc["bin Location"] = Location;

      // Log extracted values
      Serial.printf("Bin Level: %s, Model Prediction: %s, Dirtiness: %s, Bin ID: %s\n", 
                    binLevel.c_str(), status.c_str(), dirtiness.c_str(), ID.c_str());

      // Create the message string for WhatsApp
      String message = "Bin Location: " + Location + "\n" +
                       "Bin Level: " + binLevel + "%\n" +
                       "Bin Surrounding: " + status + "\n" +
                       "Dirtiness: " + dirtiness + "%";

      // Send WhatsApp message
      sendWhatsAppMessage(phoneNumber1, message.c_str());

      // Serialize JSON document to string
      String jsonString;
      serializeJson(doc, jsonString);

      // Broadcast JSON string via WebSocket
      webSocket.broadcastTXT(jsonString);

      // Send acknowledgment back to sender via LoRa
      String ackCommand = "ACK";
      lora.println("AT+SEND=2," + String(ackCommand.length()) + "," + ackCommand);
    } else {
      Serial.println("Error: Could not find 'Bin Level' in message.");
    }
  } else {
    Serial.println("Error parsing message.");
  }
}


String BinLocation(String ID){
  String Location_1="Tharapi_home";
  //you can add new location and add else if with new ID 
  if (ID=="2"){
    return Location_1;

  }
  else {
    return "NO BIN found for this ID";
  }
}

// Put ESP32 into deep sleep
void goToSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_1m);  // Wake up after 1 minute
  Serial.println("Going to sleep now");
  delay(100);  // Allow time for Serial to flush
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
