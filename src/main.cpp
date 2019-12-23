/******************************************************************************
Publishes SG and Temp readings to Google Cloud IoT Core via MQTT
 *****************************************************************************/
#include "esp32-mqtt.h"
#include <BLEDevice.h>

// User Settings
const int bleScanTime = 5;  // Duration to scan for bluetooth devices (in seconds).
const int publishTime = 3570000; // PubSub publish interval (in ms)
const bool Celsius = true;
int repeatColour = 0;
float DevGravityFormatted;
String DevColour;
float DevTemp;
unsigned long lastMillis = 0;
BLEScan* pBLEScan;

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

int parseTilt(String DevData) {
  String DevTempData, DevGravityData;
  int colourInt;
  float DevGravity;

  // Determine the Colour
  colourInt = DevData.substring(6, 7).toInt();

  switch (colourInt) {
    case 1:
      DevColour = "Red";
      break;
    case 2:
      DevColour = "Green";
      break;
    case 3:
      DevColour = "Black";
      break;
    case 4:
      DevColour = "Purple";
      break;
    case 5:
      DevColour = "Orange";
      break;
    case 6:
      DevColour = "Blue";
      break;
    case 7:
      DevColour = "Yellow";
      break;
    case 8:
      DevColour = "Pink";
      break;
  }

  if (repeatColour != 0 && repeatColour != colourInt) {
    Serial.print(DevColour);
    Serial.println(" Tilt is being ignored.");
    return 0;
  }

  // Get the temperature
  DevTempData = DevData.substring(32, 36);
  DevTemp = strtol(DevTempData.c_str(), NULL, 16); // Temp in Freedumb units

  // Get the gravity
  DevGravityData = DevData.substring(36, 40);
  DevGravity = strtol(DevGravityData.c_str() , NULL, 16);

  Serial.println("--------------------------------");
  Serial.print("Colour: ");
  Serial.println(DevColour);
  Serial.print("Temp: ");
  if (Celsius) {
    DevTemp = (DevTemp-32.0) * (5.0/9.0);
    Serial.print(DevTemp);
    Serial.println(" C");
  }
  else {
    Serial.print(DevTemp);
    Serial.println(" F");
  }
  Serial.print("Gravity: ");
  DevGravityFormatted = (DevGravity / 1000);
  Serial.println(DevGravityFormatted, 3);
  Serial.println(DevData);
  Serial.println("--------------------------------");
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
  if (millis() - lastMillis > publishTime) {
    lastMillis = millis();
    int deviceCount, tiltCount = 0;
    int colourFound = 1;
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    Serial.println();
    Serial.println("Scanning...");
    BLEScanResults foundDevices = pBLEScan->start(bleScanTime);
    deviceCount = foundDevices.getCount();
    Serial.print(deviceCount);
    Serial.println(" Devices Found.");
    pBLEScan->stop();
    BLEDevice::deinit(0);
    for (uint32_t i = 0; i < deviceCount; i++) {
      BLEAdvertisedDevice Device = foundDevices.getDevice(i);
      String DevString, DevData;
      DevString = Device.toString().c_str();
      DevData = DevString.substring(63);

      if (DevData.substring(7, 32) == "0c5b14b44b5121370f02d74de") { // Tilt found
        tiltCount++;
        Serial.print("Device #");
        Serial.print(i);
        Serial.println(" is a Tilt");
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
      Serial.println("No Tilts Found.");
      // Do something like quickly search again?
    }
    // To-Do: Change Colour to colour
    else {
      String payload;
      payload += F("{\"currentTime\":\"");
      payload += F("1970-01-01 00:00:01");
      payload += F("\",\"SG\":\"");
      payload += String(DevGravityFormatted, 3);
      payload += F("\",\"colour\":\"");
      payload += String(DevColour);
      payload += F("\",\"temperature\":\"");
      payload += String(DevTemp, 1) + "\"}";
      publishTelemetry(payload);
      Serial.println(payload);
    }
  }
}
