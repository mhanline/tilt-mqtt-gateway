/******************************************************************************
Publishes SG and Temp readings to Google Cloud IoT Core via MQTT
******************************************************************************/
#define DEBUGMACROS
#include <DebugMacros.h>
#include <BLEDevice.h>
#include <Arduino.h>
#include "tilt-gateway.pb.h"
#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include "universal-mqtt.h"
#include <WiFiManager.h>

// User Settings

#define bleScanTime (int)10 // Duration to scan for bluetooth devices in seconds
#define publishTime (int)1800000 // 30 minutes: PubSub publish interval (in ms)
#define Celsius true
#define TRIGGER_PIN 16 // AP factory reset button
#define PROTOMSGVER 1 //Bump this if changing the protobuf format
#define scanTime 10 //Time to run the BT scan in seconds

unsigned long lastMillis = 0;
BLEScan* pBLEScan;

WiFiManager wm;

void printhex(const uint8_t *readmem, const unsigned int memsize);

void checkButton();

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      uint8_t mfgData[51] = {}; //To-Do: reduce this size if necessary
      unsigned int payloadLen = advertisedDevice.getPayloadLength();
      memcpy(mfgData, advertisedDevice.getPayload() + 2, payloadLen);
      /*DPRINTF("MFG STRING: ");
      printhex(mfgData, payloadLen);*/
      if ((memcmp(mfgData, "\x4c\x00\x02\x15\xa4\x95\xbb", 7) != 0) && 
      (memcmp(mfgData + 8, "\xc5\xb1\x4b\x44\xb5\x12\x13\x70\xf0\x2d\x74\xde", 12) != 0)) {
        return;
      }
      DPRINTLNF("Found a tilt!");
      // To-Do: Validate tilt colour field
      char tiltColourNum[1];
      memcpy(tiltColourNum, mfgData + 7, sizeof(tiltColourNum));
      DPRINTFMT("Tilt Colour shift 4: %d\n", (int)tiltColourNum[0] >> 4);
      uint16_t tiltTemp;
      memcpy(&tiltTemp, mfgData + 20, sizeof(tiltTemp));
      tiltTemp = __builtin_bswap16(tiltTemp);
      DPRINTFMT("Temperature in deg F: %d\n", (int)tiltTemp);
      uint16_t tiltSG;
      memcpy(&tiltSG, mfgData + 22, sizeof(tiltSG));
      tiltSG = __builtin_bswap16(tiltSG);
      DPRINTFMT("Specific Gravity (float): %.3f\n", (float)tiltSG / 1000);
      bool status;
      uint8_t msgPayload[50];
      tiltmsg message = tiltmsg_init_zero;
      pb_ostream_t stream = pb_ostream_from_buffer(msgPayload, sizeof(msgPayload));
      time_t unixTNow;
      time(&unixTNow);
      message.version = PROTOMSGVER;
      message.specificGravity = (int)tiltSG;
      message.temperature = (int)tiltTemp;
      //If we shift 4 bytes right, we get the correct decimal num
      message.colour = tiltmsg_colour_type(tiltColourNum[0] >> 4);
      message.timeStamp = unixTNow;
      message.txPower = 1; //To-Do: Add TxPower. Enable in in debug mode.
      status = pb_encode(&stream, tiltmsg_fields, &message);
      DPRINTFMT("Bytes Written: %d \n", stream.bytes_written);
      DPRINTF("Protobuf Message: ");
      printhex(msgPayload, (unsigned int)(stream.bytes_written));
      if (!status) {
        DPRINTFMT("Encoding failed: %s\n", PB_GET_ERROR(&stream));
      }
      publishTelemetry((const char *)msgPayload, stream.bytes_written);
    }
};

void setup() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  bool res;
  res = wm.autoConnect("tilt-gw-init","tilt-gateway"); // password protected ap
  if(!res) {
      DPRINTLNF("Failed to connect");
      // ESP.restart();
  } 
  else { 
      DPRINTF("WiFi Connected...)");
  }
  setupCloudIoT();
  lastMillis = millis() + publishTime; // Starts an initial publish straight away
  BLEDevice::init("");
}

void loop() {
  checkButton();
  if (!mqttClient->connected()) {
    connect();
  }
  mqtt->loop();
  if ((millis() - lastMillis) > publishTime) {
    lastMillis = millis();
    pBLEScan = BLEDevice::getScan();
    MyAdvertisedDeviceCallbacks myCallbacks;
    pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(40);
    pBLEScan->setWindow(30);
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    DPRINTF("Devices found: ");
    DPRINTLN(foundDevices.getCount());
    DPRINTLNF("Scan done!");
    pBLEScan->clearResults();
  }
}

void printhex(const uint8_t *readmem, const unsigned int memsize) {
  if (memsize > 1) {
    DPRINTFMT("%02X", readmem[0]);
  }
  for (int i = 1; i < memsize; i++) {
    DPRINTFMT("-%02X", readmem[i]);
  }
  DPRINTLNF("");
}

void checkButton() {
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if(digitalRead(TRIGGER_PIN) == LOW ) {
      DPRINTLNF("Button Pressed");
      // still holding button for 3000 ms, reset settings
      delay(3000); // reset delay hold
      if(digitalRead(TRIGGER_PIN) == LOW ) {
        DPRINTLNF("Button Held");
        DPRINTLNF("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      DPRINTLNF("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("OnDemandAP","password")) {
        DPRINTLNF("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      }
      else {
        //if you get here you have connected to the WiFi
        DPRINTLNF("connected...yeey :)");
      }
    }
  }
}