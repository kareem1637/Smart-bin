#ifndef COMMON_H // Unique identifier for the header guard

#define COMMON_H

#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include <EEPROM.h>            // Read and write from flash memory
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include <Arduino.h>
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "all_ops_resolver.h"
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <esp_sleep.h>

#endif // COMMON_H
