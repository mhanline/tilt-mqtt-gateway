/******************************************************************************
Publishes SG and Temp readings to Google Cloud IoT Core via MQTT
 *****************************************************************************/
#include <universal-mqtt.h>
#include <BLEDevice.h>

// User Settings

#define bleScanTime (int)5 // Duration to scan for bluetooth devices in seconds
#define publishTime (int)3570000 // PubSub publish interval (in ms)
#define Celsius true

int repeatColour = 0;  //To-Do: Figure this bit out
float* DevGravity;
char* DevColour[7];
float* DevTemp;
unsigned long lastMillis = 0;
BLEScan* pBLEScan;

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");  //To-Do: Figure this out
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

int parseTilt(String DevData) {
  int colourInt;
  // Determine the Colour
  colourInt = DevData.substring(6, 7).toInt();
  switch (colourInt) {
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
  // To-Do: Use bitwise check here maybe?
  // Looks like it is checking if tilt has been detected already and ignores if so
  // But it doesn't set repeatcolour anywhere
  if (repeatColour != 0 && repeatColour != colourInt) {
    Serial.print("Device Colour detected:");
    Serial.println(*DevColour);
    Serial.println(" Tilt is being ignored.");
    return 0;
  }

  // Get the temperature
  // DevTempData = DevData.substring(32, 36);
  *DevTemp = strtol(DevData.substring(32, 36).c_str(), NULL, 16);

  // Get the gravity
  // DevGravityData = DevData.substring(36, 40);
  *DevGravity = strtol(DevData.substring(36, 40).c_str(), NULL, 16) / 1000;

  Serial.print("Colour: ");
  Serial.print("Device Colour detected:");
  Serial.println(*DevColour);
  Serial.print("Temp: ");
  #ifdef Celsius
  *DevTemp = (*DevTemp-32.0) * (5.0/9.0);
  Serial.print(*DevTemp);
  Serial.println(" C");
  #else
  Serial.print(DevTemp);
  Serial.println(" F");
  #endif
  Serial.print("Gravity: ");
  Serial.println(*DevGravity, 3);
  Serial.println(DevData);
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
    Serial.print(deviceCount);
    Serial.println(" Devices Found.");
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
      // To-Do: Quickly search again?
    }
    // To-Do: Move this up to send one message per tilt. Can't handle multiple tilts
    else {
      String payload;
      payload += F("{\"currentTime\":\"");
      payload += F("1970-01-01 00:00:01");
      payload += F("\",\"SG\":\"");
      payload += *DevGravity;  //To-Do ,3 etc
      payload += F("\",\"colour\":\"");
      payload += *DevColour;
      payload += F("\",\"temperature\":\"");
      payload += String(*DevTemp, 1) + "\"}"; //To-Do: Remove String
      publishTelemetry(payload);
      Serial.println(payload);
    }
  }
}
