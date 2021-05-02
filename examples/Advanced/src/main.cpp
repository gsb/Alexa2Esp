
#define VERSION "Advance  21/05/02  0.1.6"

//-- Version Includes -----------------------------------------------------------
//    - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu 
//    - Async Web Server
//    - Alexa2Esp enabling multiple virtual devices, each w/ 100 possible voice commands
//    - Websockets and Web Monitor with Input Field for commands
//    - FIFO pending queue and command processing
//    - File system with send-file, upload, download, delete and clear
//    - OTA for both sketch and data
//    - Local Time and Date management
//    - Timers for interval and one-up function calls
//    - TP-Link Switch controls
//-------------------------------------------------------------------------------

//-- 'Opt Out' conditionals: comment or leave out to include, default.
//#define NO_FS       // File System
//#define NO_MONITOR  // Web Page Monitor
//#define NO_WS       // Web Sockets
//#define NO_NTP      // NTP (local date and time updates)
//#define NO_OTA      // WiFi firmware and data uploads
//#define NO_TP_LINK  // Do not inclure TP-Link addon


#include <Arduino.h>
#include <credentials.h>    // FIFI_SSID and WIFI_PASS definitions.
#include <alexa2esp_core.h> // Alexa2Esp Class and initializations.
#include <support.h>        // General support methods including server.

extern bool processPending(void); // (below)

device_t default_device; // Default device object - optional;

//-------------------------------------------------------------------------------

void setup() {
  serialSetup();
  wifiSetup();
  serverSetup();
  alexa2esp.initialize();

  // Add virtual devices here.
  default_device = alexa2esp.addDevice(espName.c_str(), 33);

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
  timer.every(60000, [&](void*) -> bool {
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

  debug.printf("Setup complete.\n");
  Serial.flush();
  delay(1000); // One-second pause, before initial heap reading.
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
// Alexa's return value protocols. HTTP-GET and Websocket requests  
// (if in use) do NOT pre-initialized the targer device object. This needs
// to be done here in the user's "processPending" method. Changes to the
// Alexa received value(s) can also be made while processing the request.

bool processPending() { //...sequentially execute string actions.
  if (!pending.size()) return true; // If nothing queued, nothing to do now.
  String msg = pending.front(); //...else, get first in the FIFO queue, and
  pending.pop(); // remove it from the queue for processing here and now.

  device_t dev;
  String cmd;
  int int_cmd;

  // Now, parse the "slash seperated" command message into String tokens.
  //  - tokens[0] is a device name or command (integer 0-100 || string value)
  //  - tokens[1] is an integer or string command
  //  - tokens[2] is a string of everything else, if anything at all.
  std::vector<String> tokens; str2tokens(msg, tokens, '/',3);

  // If tokens[0] is an integer command, pre-append the default device's name.
  if (tokens[0].toInt()>0 || (char)'0'==tokens[0].charAt(0)) tokens.insert(tokens.begin(), default_device_name);

  // Now only three possibilities to resolve:
  //  - target device name / integer command (From: Alexa, HTTP GET, Websocket)
  //  - target device name / string command (From: HTTP GET, Websocket)
  //  - string command for the esp itself (From: HTTP GET, Websocket)

  cmd = tokens.size()>1?tokens[1]:tokens[0];
  int_cmd = cmd.toInt(); // 0-100 where not an integer is also 0
  dev = alexa2esp.getDeviceByName(tokens[0].c_str());

  if (dev) { // device found by name or not?
    if (int_cmd > 0 || (char)'0' == cmd.charAt(0)) { //...is valid int command.
      if (int_cmd>100) int_cmd = 100;

      // Command validation and processing int_cmd "switch...case"
      //  - process the int_cmd for the device, and
      //  - save the int_cmd value to the device object.
      switch (int_cmd) {
        case 0:
          {
            alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
            debug.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
            return true;
          }

        case 33:
          {
            alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
            debug.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
            return true;
          }

        case 100:
          {
            alexa2esp.setPercent(dev, int_cmd); // Save new values in the device object.
            debug.printf("Processing: [%d] for %s\n", int_cmd, tokens[0].c_str());
            return true;
          }

        default:
          {
            debug.printf("int_cmd %d not found for %s\n", int_cmd, tokens[0].c_str());
            return false;
          }
      }

    } else { // Process tokens[1] as a string command.

      if (cmd.equals("on")) {
        debug.printf("Turn Switch ON\n");
        return true;
      }

      if (cmd.equals("off")) {
        debug.printf("Turn Switch OFF\n");
        return true;
      }

      // ...more as needed.
    }

  } else {
    //...else, device not found, so perhaps is
    // simply a string command for the ESP itself.

    if (cmd.equals("info")) {
      debug.printf("%s\n",VERSION);
      return true;
    }

    if (cmd.equals("list")) {
      String listing = listFiles();
      debug.printf("FILES:\n%s\n",listing.c_str());
      return true;
    }

    // More like: if "equals || startsWith" then do this {...} and return true

    if (cmd.equals("reboot")) {
      debug.printf("\nRebooting...\n");
      delay(1000);
      ESP.restart();
    }
  }

  debug.printf("Unknown command: %s\n", msg.c_str());
  return false; // No matching command with processing code found.
}

//-------------------------------------------------------------------------------