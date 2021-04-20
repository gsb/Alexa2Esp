
#define VERSION "Alexa2Esp Starter  21/04/18P  0.1.2"


//-- Version Includes -----------------------------------------------------------
//    - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu 
//    - Async Web Server
//    - Alexa2Esp enabling 2 virtual devices, both w/ 101 possible voice commands
//-------------------------------------------------------------------------------

#include <Arduino.h>
#include <credentials.h>    // FIFI_SSID and WIFI_PASS definitions.
#include <alexa2esp_core.h> // Alexa2Esp Class and initializations.


extern void serialSetup(void);
extern void wifiSetup(void);
extern void serverSetup(void);


//-------------------------------------------------------------------------------

void setup() {
    serialSetup();
    wifiSetup();
    serverSetup();
    alexa2esp.initialize();

    alexa2esp.onSetState([](device_t dev) {
        // Common device callback. Passed the associate alexa2esp's
        // vertual device object reference pre-initialized with
        // the percent and value based on the Alexa voice command.
        Serial.printf("User Callback:  deviceName: %s,  percent: %u,  value: %u\n", dev->name, dev->percent, dev->value);
    });

    // Add virtual device(s) by name. Default is first added.
    alexa2esp.addDevice(espName.c_str(),25);
    Serial.flush();
}

void loop() {
  alexa2esp.handle();  // Manage Alexa connection for UDP etc.
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

//-------------------------------------------------------------------------------