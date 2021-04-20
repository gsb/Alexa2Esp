
#define VERSION "Alexa2Esp32  21/04/18P  0.1.2"

//-- Version Includes -----------------------------------------------------------
//    - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu 
//    - Alexa2Esp enabling two virtual devices, both w/ 101 voice commands
//    - Websockets and Web Monitor with Input Field for commands
//    - FIFO pending queue and command processing
//    - File system with send-file, upload, download, delete and clear
//    - OTA for both sketch and data
//    - Local Time and Date management
//    - Timers for interval and one-up function calls
//    - TP-Link Switch controls
//-------------------------------------------------------------------------------

//-- Opt out conditionals: comment or leave out to include...
//#define NO_FS       // File System
//#define NO_MONITOR  // Web Page Monitor
//#define NO_WS       // web sockets server
//#define NO_NTP      // NTP [local] date and time updates


#include <Arduino.h>
#include <credentials.h>        // FIFI_SSID and WIFI_PASS definitions.
#include <alexa2esp_core.h>     // Alexa2Esp Class and initializations.
#include <alexa2esp_support.h>  // General support methods including server.

extern bool processPending(void); // (below)

//-------------------------------------------------------------------------------

void setup() {
  serialSetup();
  wifiSetup();
  serverSetup();
  otaSetup();
  alexa2esp.initialize();

  // Add virtual devices here.
  alexa2esp.addDevice(espName.c_str(), 75);

  // LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Our LED_BUILTIN has inverse logic (high for OFF, low for ON)

  //== SETUP TIMERS ==========================
  // POL "proof-of-life" Timer
  pinMode(LED_BUILTIN, OUTPUT); // POL Blinker...
  timer.every(500, [&](void*) -> bool {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    return true; // repeat? true
  });
  timer.every(10000, [&](void*) -> bool {
    char buff[64];
    snprintf(buff, sizeof(buff),
      "init: %lu  curr: %u  delta: %d\n",
        init_heap_size,
        ESP.getFreeHeap(),
        (int)(ESP.getFreeHeap()-init_heap_size)
    );
    #ifdef ARDUINO_ARCH_ESP32
      tmstruct.tm_year = 0;
      getLocalTime(&tmstruct, 5000);
      debug.printf("heap: %02d:%02d:%02d  ",tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
    #else
      debug.printf("heap: %s  ",timeClient.getFormattedTime().c_str());
    #endif
    debug.printf("%s", buff);
    return true; // repeat? true
  });
  //==========================================

  delay(1000);
  debug.printf("Setup complete.\n");
  Serial.flush();
  init_heap_size = ESP.getFreeHeap();
}

void loop() {
  timer.tick();        // Check timers and run those ready.
  alexa2esp.handle();  // Manage Alexa connection for UDP etc.
  processPending();    // Check pending command queue and process first. 
  ArduinoOTA.handle(); // Process OTA application code and/or data.
  #ifndef NO_WS
    ws.cleanupClients(); // Websockets maintenance loop.
  #endif
  #if !defined(NO_NTP) && !defined(ARDUINO_ARCH_ESP32)
    timeClient.update();// ...update date and time as needed from the pool.
  #endif
}

//-------------------------------------------------------------------------
//-- Process Pending Queue Command Processing and Support methods. --------
//-------------------------------------------------------------------------
//-- Process all command messages. Expecting a formatted command message:
//   1 - alexa2esp String device name and String command, like "on"/"off",
//   2 - alexa2esp String device name and integer command, 0-100, OR
//   3 - String command, like "info" or "reboot" targeting the ESP itself.
//
// The associated Alexa voice command target device object is already
// pre-initialized with latest percent and value for consistency with
// Alexa's return value protocols. HTTP-GET and Web Socket requests  
// (if in use) do NOT pre-initialized the targer device object. This
// needs to be done here in the user's "processPending" method. Changes
// to the received value(s) can also be made while processing the request.

bool processPending() { //...sequentially execute string actions.
  if (!pending.size()) return true; //...nothing queued.

  // Things needed.
  device_t dev;
  String cmd;
  int int_cmd;

  //...get first in queue - FIFO queue.
  String msg = pending.front();
  pending.pop();

debug.printf("processing:  %s\n", msg.c_str());

  // Parse the command message into slash seperated tokens.
  std::vector<String> tokens;
  str2tokens(msg, tokens, '/',3);  // Tokens[2] is remaining data, if any.
  if (tokens.size()==1 && (tokens[0].toInt()>0 || (char)'0'==tokens[0].charAt(0))) {
    tokens.insert(tokens.begin(), espName); //...asume int_cmd is for default device.
  }
  //for(size_t i=0; i<tokens.size(); i++) debug.printf(" %d of %d - %s\n", i, tokens.size(), tokens[i].c_str());
  cmd = tokens.size()>1?tokens[1]:tokens[0];
  int_cmd = cmd.toInt();
  dev = alexa2esp.getDeviceByName(tokens[0].c_str());
  if (dev) { // device found
    if (int_cmd > 0 || (char)'0' == cmd.charAt(0)) { //...is valid int command.
      if (int_cmd>100) int_cmd = 100;
      switch (int_cmd) {
        case 0:
          {
            // do something based on int_cmd.
            debug.printf("Processing zero: [%d] for %s\n", int_cmd, tokens[0].c_str());
            dev->percent = int_cmd; // Set a new value (int_cmd) in the device object.
            return true;
          }

        // ...do something based on int_cmd's value switch-case.

        default:
          { // example for all but 0

            alexa2esp.setPercent(dev, int_cmd);
            debug.printf("Processing: %d for %s\n", int_cmd, tokens[0].c_str());
            return true;

            /* really default is... */
            debug.printf("int_cmd %d not found for %s\n", int_cmd, tokens[0].c_str());
            return false;
          }
      }

    } else { // Process tokens[1] as a string command.
      String str_cmd = tokens[1];

      if (str_cmd.equals("on")) {
        debug.printf("Turn Switch ON\n");
        return true;
      }

      if (str_cmd.equals("off")) {
        debug.printf("Turn Switch OFF\n");
        return true;
      }

      // ...as needed.

    }
    return false;
  }

  //...else perhaps, simply a string command for the ESP itself.

  if (tokens[0].equals("info")) {
    debug.printf("%s\n",VERSION);
    return true;
  }

  if (tokens[0].equals("list")) {
    String listing = listFiles();
    debug.printf("%s\n",listing.c_str());
    return true;
  }

  if (tokens[0].equals("reboot")) {
    debug.printf("\nRebooting...\n");
    delay(1000);
    ESP.restart();
  }

  // More like: if "equals || startsWith" then do this {...}

  debug.printf("Unknown command: %s\n", msg.c_str());
  return false; // No matching processing code found.
}

//-------------------------------------------------------------------------------