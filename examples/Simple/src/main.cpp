
#define VERSION "Simple  21/05/02  0.1.6"

//-- Version Includes -----------------------------------------------------------
//    - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu 
//    - Async Web Server
//    - Alexa2Esp enabling two virtual devices, both w/ 101 voice commands
//-------------------------------------------------------------------------------

#include <Arduino.h>
#include <credentials.h>    // FIFI_SSID and WIFI_PASS definitions.
#include <alexa2esp_core.h> // Alexa2Esp Class and initializations.


extern void serialSetup(void);
extern void wifiSetup(void);
extern void serverSetup(void);

extern bool processPending(void);


//-------------------------------------------------------------------------------

void setup() {
    serialSetup();
    wifiSetup();
    serverSetup();
    alexa2esp.initialize();

    // Add virtual device(s) by name. Default device is first added.
    alexa2esp.addDevice(espName.c_str(),33);
    Serial.flush();
}

void loop() {
  alexa2esp.handle(); // Manage Alexa connection for UDP etc.
  processPending();   // Check pending command queue and process first. 
}


//-- Support Methods ------------------------------------------------------------

//-- Serial Setup
void serialSetup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  while(!Serial){delay(100);};
  Serial.printf("\n\nVersion: %s\n", VERSION);
  Serial.flush();
}


//-- WIFI Setup
void wifiSetup() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS); //...see credentials.h
  Serial.print("\nConnecting to WiFi..."); // Wait for connection.
  for(uint8_t count=0; WiFi.status() != WL_CONNECTED; count++) {
    delay(500);
    Serial.print(".");
    if (count>19) { // If not connected within 19 tries, quit and reboot.
      Serial.println("\n[WIFI] Connection Failed! Rebooting...\n");
      ESP.restart();
    }
  }
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString().c_str());
  if(!(espName.length()))espName="esp"+String(WiFi.localIP()[3]);
  Serial.printf("ESP Name: %s\n\n", espName.c_str());
}


//-- Server Setup
void serverSetup() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    #ifdef VERSION
      Serial.println(VERSION);
      request->send(200, "text/plain", VERSION);
    #else
      Serial.println("Esp2Alexa Starter Example");
      request->send(200, "text/plain", "Esp2Alexa Starter Example");
    #endif
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (alexa2esp.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data))) return;
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
      String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
      if (alexa2esp.process(request->client(), request->method() == HTTP_GET, request->url(), body)) return;
      request->send(200, "text/plain", "Unknown: "+request->url());
  });

  server.begin();  // Start the server
}


//-- processPending -------------------------------------------------------------
//   The Command Messages are processed here sequentially, taken from the  
//   FIFO queue, 'pending' one per time loop. Alexa2Esp queues up formatted 
//   messages as: device_name/command, where command is an integer value.

bool processPending() { // Sequentially process string commands from FIFO queue.
  if (!pending.size()) return true; // If nothing queued, no processing for now.
  String msg = pending.front(); //...Else, get first in the FIFO queue, and
  pending.pop(); // remove it from the queue for processing here and now.

  // Now, parse the slash seperated command message into String tokens.
  std::vector<String> tokens;     // tokens[0] - device name,
  str2tokens(msg, tokens, '/',2); // tokens[1] - command as integer, 0-100.

  device_t dev = alexa2esp.getDeviceByName(tokens[0].c_str());
  uint8_t int_cmd = tokens[1].toInt();

  // Command validation and processing int_cmd "switch...case"
  //  - process the int_cmd for the device, and
  //  - save the int_cmd value to the device object.
  switch (int_cmd) {
    case 0:
      {
        alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
        Serial.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
        return true;
      }

    case 33:
      {
        alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
        Serial.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
        return true;
      }

    case 100:
      {
        alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
        Serial.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
        return true;
      }

    default:
      {
        Serial.printf("int_cmd %d not found for %s\n", int_cmd, tokens[0].c_str());
        return false;
      }
  }
}

//-------------------------------------------------------------------------------