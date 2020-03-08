/******************************************************************************
Publishes SG and Temp readings to Google Cloud IoT Core via MQTT
 *****************************************************************************/
#define DEBUGMACROS
#include <DebugMacros.h>
#include <BLEDevice.h>
#include <Arduino.h>
#include "universal-mqtt.h"
#include "tilt-gateway.pb.h"
#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"

// User Settings

#define bleScanTime (int)5 // Duration to scan for bluetooth devices in seconds
#define publishTime (int)3570000 // PubSub publish interval (in ms)
#define Celsius true

int repeatColour = 0;
float* DevGravity;
uint8_t* devColourNum;
char* DevColour[7];
float* DevTemp;
unsigned long lastMillis = 0;
BLEScan* pBLEScan;

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    DPRINTF("Failed to obtain time");
    return;
  }
  DPRINTLN(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

void messageReceived(String &topic, String &payload) {
  DPRINTLN("incoming: " + topic + " - " + payload);
}

int parseTilt(String DevData) {
  // int colourInt;
  // Determine the Colour
  *devColourNum = DevData.substring(6, 7).toInt();
/*  switch (colourInt) {
    case 1:
      strcpy(*DevColour,"Red");
      break;
    case 2:
      strcpy(*DevColour,"Green");
      break;
    case 3:
      strcpy(*DevColour,"Black");
      break;
    case 4:
      strcpy(*DevColour,"Purple");
      break;
    case 5:
      strcpy(*DevColour,"Orange");
      break;
    case 6:
      strcpy(*DevColour,"Blue");
      break;
    case 7:
      strcpy(*DevColour,"Yellow");
      break;
    case 8:
      strcpy(*DevColour,"Pink");
      break;
  }
  if (repeatColour != 0 && repeatColour != colourInt) {
    DPRINT("Device Colour detected:");
    DPRINTLN(*DevColour);
    DPRINTLNF(" Tilt is being ignored.");
    return 0;
  } */

  // Get the temperature
  // DevTempData = DevData.substring(32, 36);
  *DevTemp = strtol(DevData.substring(32, 36).c_str(), NULL, 16);

  // Get the gravity
  // DevGravityData = DevData.substring(36, 40);
  *DevGravity = strtol(DevData.substring(36, 40).c_str(), NULL, 16) / 1000;

  DPRINTF("Device Colour detected:");
  DPRINTLN(*DevColour);
  DPRINTF("Temp: ");
  #ifdef Celsius
  *DevTemp = (*DevTemp-32.0) * (5.0/9.0);
  DPRINT(*DevTemp);
  DPRINTLNF(" C");
  #else
  DPRINT(DevTemp);
  DPRINTLNF(" F");
  #endif
  DPRINTF("Gravity: ");
  DPRINTLN(*DevGravity, 3);
  DEBUG_PRINTLN(DevData);
  return 2;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setupCloudIoT();
  lastMillis = millis() + publishTime; // Starts an initial publish straight away
}

void loop() {
  mqtt->loop();
  delay(10);  // <- fixes some issues with WiFi stability
  if (!mqttClient->connected()) {
    connect();
  }
  if ((millis() - lastMillis) > publishTime) {
    lastMillis = millis();
    int deviceCount, tiltCount = 0;
    int colourFound = 1;
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    BLEScanResults foundDevices = pBLEScan->start(bleScanTime);
    deviceCount = foundDevices.getCount();
    DPRINT(deviceCount);
    DPRINTLNF(" Devices Found.");
    pBLEScan->stop();
    BLEDevice::deinit(0);
    for (uint32_t i = 0; i < deviceCount; i++) {
      BLEAdvertisedDevice Device = foundDevices.getDevice(i);
      String DevString;
      String DevData;
      DevString = Device.toString().c_str();
      DevData = DevString.substring(63);

      if (DevData.substring(7, 32) == "0c5b14b44b5121370f02d74de") { // Tilt found
        tiltCount++;
        DPRINTF("Device #");
        DPRINT(i);
        DPRINTLNF(" is a Tilt");
        int tiltSuccess = parseTilt(DevData);
        if (tiltSuccess == 2) {
          colourFound = 2;
        }
        if (!tiltSuccess && colourFound != 2) {
          colourFound = 0;
        }
      }
    }
    if (!tiltCount || !colourFound) {
      DPRINTLNF("No Tilts Found.");
    }
    else {
      bool status;
      uint8_t strPayload[120];
      char strDevGravity[8];
      dtostrf(*DevGravity, 4, 3, strDevGravity);
      char strDevTemp[8];
      dtostrf(*DevTemp, 4, 2, strDevTemp);
      tiltmsg message = tiltmsg_init_zero;
      pb_ostream_t stream = pb_ostream_from_buffer(strPayload, sizeof(strPayload));
      message.specificGravity = *DevGravity;
      message.temperature = *DevTemp;
      message.colour = tiltmsg_colour_type(*devColourNum);
      message.timeStamp = 0;
      status = pb_encode(&stream, tiltmsg_fields, &message);
      DPRINTF("Bytes Written: ");
      DPRINTLN(stream.bytes_written);
      DPRINTF("Message: ");
      for(int i = 0; i<stream.bytes_written; i++){
        DPRINTFMT("%02X",strPayload[i]);
      }
      if (!status) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
      }
      // publishTelemetry(strPayload);
      // DEBUG_PRINTLN(strPayload);
    }
  }
}
