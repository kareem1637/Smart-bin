#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "common.h"
#include "Camera_PINS.h"

// Sleep durations in microseconds
#define SLEEP_12h 43200000000ULL  // 12 hours
#define SLEEP_1h 3600000000ULL    // 1 hour
#define SLEEP_5m 300000000ULL     // 5 minutes
#define SLEEP_1m 60000000ULL      // 1 minute


// ESP-NOW parameters
extern uint8_t espMACAddress[];
extern uint8_t espCamMACAddress[];
extern esp_now_peer_info_t peerInfo;

// Size of a single packet for ESP-NOW
typedef struct struct_message {
  char data[100];  // Data buffer for messages
} struct_message;

extern struct_message Sent_Data;
extern struct_message Signal;
extern bool sleepSignal;
extern bool Send_pred;
extern int pictureNumber;
extern String jpegPath;
extern File jpegFile;
extern camera_fb_t* fb;
extern unsigned long long sleepTime;
extern bool sleepFlag;

// Callback function for ESP-NOW
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len);

// Initialization functions
void init_EspNow();
void init_SD();
void init_EEPROM();

// Helper functions
camera_fb_t* captureAndOpenFiles();
void printSleepTime(unsigned long long microseconds);
void goToSleep();
void SendPred();
void captureImage();
void savePictureNumber();

#endif  // CONSTANTS_H
