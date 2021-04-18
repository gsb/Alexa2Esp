
#ifndef ALEXA2ESP_SUPPORT_H
#define ALEXA2ESP_SUPPORT_H


#ifndef debug
  #define debug Serial
#endif

#ifndef SERIAL_BAUD_RATE
  #define SERIAL_BAUD_RATE 115200
#endif

#ifndef SERVER_PORT
  #define SERVER_PORT 80
#endif


#include <Arduino.h>
#include <credentials.h>

#include <time.h>
#include <arduino-timer.h>

//...included in core.h, here for reference.
//#include <vector>
//#include <queue>
//#include <MD5Builder.h>
//#include <ESPAsyncWebServer.h>
//#include <WiFiUdp.h> //...not async.

#include <ArduinoOTA.h>

#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #ifndef NO_NTP
    #include <time.h>
    struct tm tmstruct;
  #endif
  #ifndef NO_FS
    #include <Update.h>
    #include "FS.h"
    #include "SPIFFS.h"
    #define  FILES SPIFFS
    #define  U_PART U_SPIFFS
  #endif
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #ifndef NO_NTP
    #include <NTPClient.h>
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "us.pool.ntp.org", -14400, 60000);
  #endif
  #ifndef NO_FS
    #include <Updater.h>
    #include "FS.h"
    #include "LittleFS.h"
    #define FILES LittleFS
    #define U_PART U_FS
  #endif
#endif

//-- Debug Printing to Web Page Monitor
#ifndef debug
  #define debug Serial
#endif

#ifndef NO_WS
  AsyncWebSocket ws("/ws"); // Websocket interface
#endif

#if !defined(NO_MONITOR) && !defined(NO_WS)
  uint8_t monitorId;
  class Debug {
    public:
      void printf(const char * sw_formatting,...) {
        static char ws_buffer[1024];
        size_t len = sizeof(ws_buffer)-1;
        va_list ws_argptr;
        va_start(ws_argptr, sw_formatting);
        vsnprintf(&ws_buffer[0], len, sw_formatting, ws_argptr);
        va_end(ws_argptr);
        ws_buffer[len] = '\0';
        if (monitorId) ws.text(monitorId, ws_buffer);
        Serial.printf("%s",ws_buffer);
        Serial.flush();
      }
      void flush(void) {Serial.flush();}
  };
  Debug DBG;
  #undef debug
  #define debug DBG
#endif


//-- Globals, Prototypes and such...
#ifndef CORE_SUPPORT_H
  std::queue<String> pending; // Command pending queue, FIFO, for better time sequencing.
  AsyncWebServer server(SERVER_PORT); // Async server interface
  String espName; // The ESP Name based it's IP address (hopefully fixed.)
#endif


//-- Globals, Prototypes and such...
auto timer = timer_create_default();
long unsigned init_heap_size = 0;


#ifndef NO_FS
  PROGMEM const char uploadPage[] = "<h2>File Uploader</h2><form method=\"POST\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\" onchange=\"document.getElementsByTagName('input')[1].click()\"><input style=\"display:none;\" type=\"submit\" value=\"upload\"></form>";
#endif

#if !defined(NO_MONITOR) && !defined(NO_WS)
//-- Monitor Web Page HTML
const char monitor_html[] PROGMEM = R"rawliteral(
<style>
  *{box-sizing:border-box;}:focus{outline:none;}
  body{overflow-y:hidden;}
  input{font-size:18px;width:calc(99vw - 6px);height:35px;padding:5px;align-self:center;}
  textarea{resize:none;border:none;width:calc(99vw - 6px);height:calc(99vh - 46px);padding:10px;align-self:center;}
</style>
<script>
  window.$ = document.querySelector.bind(document);
  $.gateway = 'ws://'+window.location.host+':80/ws';
  $.stb = function() {
    var toBottom = function(){$.textarea.scrollTop=$.textarea.scrollHeight-50;};
    setTimeout(toBottom, 0);
  }
  $.addMessage = function(m) {
    $.textarea.value += m; /* (m+"\r\n") */
    $.stb();
  }
  $.restartSocket = function() {
    clearTimeout($.restartSocketId);
    try{$.ws.send("html");}catch(e){
      if (!$.ws) {
        $.startSocket();
        return;
      }
    };
    $.restartSocketId = setTimeout($.restartSocket, 2000);
  }
  $.startSocket = function() {
    if ($.ws) {
      $.ws.close();
      $.ws = null;
    }
    $.ws = new WebSocket($.gateway);
    $.ws.onopen = function(e) {
      $.addMessage("Monitor opened.\n");
      $.ws.send("monitor");
    };
    $.ws.onclose = function(e) {
      $.addMessage("Monitor disconnected.\n");
      setTimeout($.startSocket, 1000);
    };
    $.ws.onerror = function(e) {
      $.addMessage("Monitor Error:  " + e.message + "\n")
      $.ws.close();
      $.ws = null;
    };
    $.ws.onmessage = function(e) {
      $.addMessage(e.data);
    };
    $.input.onkeydown = function(e) {
      if(e.keyCode == 13 ) {
        if ($.input.value == "") {
          $.textarea.value = "cleared\n";
        } else {
          $.addMessage(">> "+$.input.value+"\n");
          $.ws.send($.input.value);
        }
        $.input.value = "";
      }
    }
  }
  window.onload = function() {
    $.input = $('input');
    $.textarea = $('textarea');
    $.startSocket();
    $.restartSocket();
  }
</script>
<body>
  <label><input type="text" placeholder=">>"/></label>
  <textarea onClick=""></textarea>
</body>
)rawliteral";

#endif


//-- Support Methods ------------------------------------------------------------

//-- Serial Setup
void serialSetup(void) {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setDebugOutput(false);
  while(!Serial){delay(100);};
  debug.printf("\n\nVersion: %s\n", VERSION);
  Serial.flush();
}

//-- WIFI Setup
void wifiSetup() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  debug.printf("\nConnecting to WiFi..."); // Wait for connection.
  for(uint8_t count=0; WiFi.status() != WL_CONNECTED; count++) {
    delay(500);
    Serial.print(".");
    if (count>19) { // If not connected within 19 tries, quit and reboot.
      debug.printf("\n[WIFI] Connection Failed! Rebooting...\n");
      ESP.restart();
    }
  }
  debug.printf("Connected to ");
  debug.printf(WIFI_SSID);
  debug.printf("\nIP address: ");
  debug.printf(WiFi.localIP().toString().c_str());
  if(!(espName.length()))espName="esp"+String(WiFi.localIP()[3]);
  debug.printf("\nESP Name: %s\n\n", espName.c_str());

  #ifndef NO_NTP
    //-- Time and Date management.
    #ifdef ARDUINO_ARCH_ESP32
      Serial.print("Contacting Time Server");
      // const int timeZone = -18000; // Eastern Standard Time, -5*3600
      // const int timeZone = -14400; // Daylight Savings Time Offset, (0|1)*3600
      configTime(-14400, 0, "us.pool.ntp.org"); //, "us.pool.ntp.org", "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
//      struct tm tmstruct;
      delay(2000);
      tmstruct.tm_year = 0;
      getLocalTime(&tmstruct, 5000);
      Serial.printf("   Now is : %d-%02d-%02d %02d:%02d:%02d\n\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
    //#else
      //NTPClient timeClient(ntpUDP, "us.pool.ntp.org", -14400, 60000);
      //delay(2000);
      //Serial.println(timeClient.getFormattedTime());
    #endif
  #endif

  #ifndef NO_FS
    if (!FILES.begin()) {
      debug.printf("File System Mount Failed. Re-build and upload file image.");
    }
  #endif
}

//-- String to String Tokens Vector. Non-destructive.
void str2tokens(String data, std::vector<String> &tokens, const char delim = '/', size_t max = 0) {
  size_t i=0;
  while(i<=data.length() && data.charAt(i)==delim) { ++i; } //...strip leading /'s
  for (size_t n=i; i <= data.length(); ++i) {
    if ((data.charAt(i) == delim) || (i == (data.length()))) {
      tokens.push_back(data.substring(n,i));
      if (max && tokens.size() == max-1) {
        tokens.push_back(data.substring(i+1)); //...remaining as last.
        return;
      }
      n = i+1;
    }
  }
  // USAGE:
  // std::vector<String> tokens;      // tokens list object.
  // str2tokens(msg, tokens, '/', 3); // 0,1,2 where tokens[2] is remaining.
}
String tokens2str(std::vector<String> &tokens, const char delim = '/', size_t start=0, bool wrap=false) {
  String str = "";
  if(wrap)str += delim; //...perhaps, add leading 'delim'.
  for (size_t i=start; i < tokens.size() ; i++) {
    if (i) str += delim;
    str += tokens[i];
  }
  if(wrap)str += delim; //...add ending 'delim', or not.
  return str;
}

#ifndef NO_WS
// Callback for incoming event
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, 
        AwsEventType type, void * arg, uint8_t *data, size_t len)
  {
    switch(type) {

      case WS_EVT_CONNECT:
          debug.printf("Connected: id:%u  IP:%s\n", client->id(), client->remoteIP().toString().c_str());
          break;

      case WS_EVT_DISCONNECT:
          monitorId = 0;
          debug.printf("Disconnected: id:%u\n", client->id());
          break;

      case WS_EVT_DATA:
        {
          AwsFrameInfo *info = (AwsFrameInfo*)arg;
          if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            String msg;
            for (size_t i=0; i < len; i++) msg += (char)data[i];
            if (msg.equals("html")) return;
            if (msg.equals("monitor")){monitorId=client->id();return;}
            pending.push(msg);
          }
        }
        break;

      case WS_EVT_PONG:
          debug.printf("Pong:\n\tClient id: %u\n", client->id());
          break;

      case WS_EVT_ERROR:
          debug.printf("Error:\n\tClient id: %u\n", client->id());
          break;   
    }
}
#endif

//-- OTA Setup
void otaSetup(void) {
  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else { // U_SPIFFS
        debug.printf("OTA uploading data.");
        Serial.flush();
        type = "filesystem";
        FILES.end();
      }
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      debug.printf("Start updating %s\n",type.c_str());
    });
    ArduinoOTA.onEnd([]() {
      debug.printf("\nEnd\n");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      debug.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      debug.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) debug.printf("Auth Failed\n");
      else if (error == OTA_BEGIN_ERROR) debug.printf("Begin Failed\n");
      else if (error == OTA_CONNECT_ERROR) debug.printf("Connect Failed\n");
      else if (error == OTA_RECEIVE_ERROR) debug.printf("Receive Failed\n");
      else if (error == OTA_END_ERROR) debug.printf("End Failed\n");
    });
  ArduinoOTA.begin();
}

//-- Server Support Methods -----------------------------------------------------

#ifndef NO_FS
  //-- Determine File MIME Type
  String getContentType(String filename) {
    if (filename.endsWith(".htm")) {
      return "text/html";
    } else if (filename.endsWith(".html")) {
      return "text/html";
    } else if (filename.endsWith(".css")) {
      return "text/css";
    } else if (filename.endsWith(".js")) {
      return "application/javascript";
    } else if (filename.endsWith(".png")) {
      return "image/png";
    } else if (filename.endsWith(".gif")) {
      return "image/gif";
    } else if (filename.endsWith(".jpg")) {
      return "image/jpeg";
    } else if (filename.endsWith(".ico")) {
      return "image/x-icon";
    } else if (filename.endsWith(".xml")) {
      return "text/xml";
    } else if (filename.endsWith(".pdf")) {
      return "application/x-pdf";
    } else if (filename.endsWith(".zip")) {
      return "application/x-zip";
    } else if (filename.endsWith(".gz")) {
      return "application/x-gzip";
    }
    return "text/plain";
  }

  //-- Handle File Send
  bool handleFileRequest( AsyncWebServerRequest *request ) {
    String path = request->url();
    if (path.endsWith("/")) path += "index.html";
    String gzPath = path + ".gz";
    if (FILES.exists(gzPath)) {
      debug.printf("Send File: %s\n",path.c_str());
      request->send(FILES, gzPath.c_str(), (getContentType(gzPath)).c_str());
      return true;
    } else if (FILES.exists(path)) {
      debug.printf("Send File: %s\n", path.c_str());
      request->send(FILES, path.c_str(), (getContentType(path)).c_str());
      return true;
    }
    return false;
  }

  #if ARDUINO_ARCH_ESP32
    //-- Handle File Listing for ESP32
    String listFiles(const char * dirname="/", uint8_t levels=3) {
      File root = FILES.open(dirname);
      if (!root) {debug.printf("Failed to open directory");return "";}
      if (!root.isDirectory()) {debug.printf("Not a directory");return "";}

      #ifndef NO_NTP
        struct tm * tmstruct;
        time_t t;
      #endif

      static String listing = "";
      File file = root.openNextFile();
      while (file) {
        if (file.isDirectory()) {
          char buffer[124];
          #ifndef NO_NTP
            t = file.getLastWrite();
            tmstruct = localtime(&t);
            snprintf(buffer,124," Dir:  %s  %d  %d-%02d-%02d %02d:%02d:%02d\n",
                file.name(), file.size(),
                (tmstruct->tm_year)+1900, (tmstruct->tm_mon)+1, tmstruct->tm_mday,
                tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec
              );
          #else
            snprintf(buffer,124," Dir:  %s  %d \n", file.name(), file.size());
          #endif
          listing += buffer;
          if (levels) listFiles(file.name(), levels -1);
        } else { //...is a standard file.
          char buffer[124];
          #ifndef NO_NTP
            t = file.getLastWrite();
            tmstruct = localtime(&t);
            snprintf(buffer,124,"File:  %s  %d  %d-%02d-%02d %02d:%02d:%02d\n",
                file.name(), file.size(), (tmstruct->tm_year)+1900, (tmstruct->tm_mon)+1, tmstruct->tm_mday,
                tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec
              );
          #else
            snprintf(buffer,124,"File:  %s  %d\n", file.name(), file.size());
          #endif

          listing += buffer; // Accumulate listing.
        }
        file = root.openNextFile();
      }
      String response = listing; //...return value is the String listing.
      listing = "";        //...clear for next use.
      return response;
    }
  #else
    String listFiles(const char * dirname="/", uint8_t levels=0) {
      String response = "";
      Dir dir = FILES.openDir("/");
      while (dir.next()) {response += "  - " + dir.fileName() + " / " + dir.fileSize() +  "\n";}
      return response;
    };
  #endif

  //-- Handle File Upload
  void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile; // File object to store the uploading file.
    if (!index) {
      if (!filename.startsWith("/")) filename = "/"+filename;
      uploadFile = FILES.open(filename, "w"); // Create persistent File object.
    }
    if ( uploadFile ) uploadFile.write(data, len);
    if ( final ) {
      if (uploadFile) {
        uploadFile.close();
        request->send(200, "text/plain", "OK");
      } else {
        debug.printf("File upload failed.\n");
        request->send(500, "text/plain", "500: file upload error.");
      }
    }
  }

  //-- Server 'on starts with' extensions
  void handleDownload(AsyncWebServerRequest *request) {
    String target = request->url().substring(9); // /download/ and leave leading /
    while(target.endsWith("/"))target=target.substring(0,target.length()-1); // remove trailing /'s
    if ( target.length() > 0 ) {
      if (FILES.exists(target)) request->send(FILES, target.c_str(), "application/octet-stream");
      else request->send(404, "text/plain", "404: Not Found ");
    } else request->send( 400, "text/plain", "Download problem." );
    return; //...done.
  }

  void handleDelete(AsyncWebServerRequest *request) {
    String target = request->url().substring(7); // /deleate/ and leave leading /
    while ( target.endsWith("/") ) target = target.substring(0,target.length()-1);
    if ( target.length() > 0 ) {
      if ( FILES.exists(target) ) {
        FILES.remove(target);
        request->send( 200, "text/plain", target + " deleted." );
      } else request->send( 400, "text/plain", "Delete problem." );
    } else request->send( 400, "text/plain", "Delete problem." );
    return; //...done.
  }

  void handleClear(AsyncWebServerRequest *request) { // /clear/ and leave leading /
    String target = request->url().substring(6);
    while ( target.endsWith("/") ) target = target.substring(0,target.length()-1);
    if ( FILES.exists(target) ) {
      File file = FILES.open(target, "w");
      file.seek(0, SeekSet);
      file.close();
      request->send( 200, "text/plain", target + " cleared." );
    } else request->send( 400, "text/plain", "Clear problem." );
    return; //...done.
  }
  
  #ifdef ARDUINO_ARCH_ESP32
    // Add ESP32 specific file handlers/support here.
  #else
    // Add ESP8266 specific file handlers/support here.
  #endif

#endif


//-- Server Setup
void serverSetup() {

  #ifdef VERSION
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      debug.printf("%s\n",VERSION);
      request->send(200, "text/plain", VERSION);
    });

    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
      debug.printf("%s\n",VERSION);
      request->send(200, "text/plain", VERSION);
    });
  #endif

  #if !defined(NO_MONITOR) && !defined(NO_WS)
    server.on("/monitor", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", monitor_html);
    });
  #endif

  #ifndef NO_FS
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK:favicon");
    });

    server.on("/list", HTTP_GET, [=](AsyncWebServerRequest *request) {
      String list = listFiles();
      debug.printf(list.c_str());
      request->send(200, "text/plain", list.c_str());
    });

    server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", uploadPage);
    });
    server.onFileUpload(handleFileUpload);//...processing.
    server.on("/upload", HTTP_POST, [&](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", "OK");
    }, handleFileUpload);
  #endif

  // Add more 'server-on' directives here, as needed.

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (alexa2esp.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data))) return;
      // Handle any other body request here...
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
      String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
      if (alexa2esp.process(request->client(), request->method() == HTTP_GET, request->url(), body)) return;

      if (!(request->url().startsWith("/api"))) {
        #ifndef NO_FS
          if (request->url().startsWith("/upload")){request->send(200,"text/plain","OK");return;}
          if (request->url().startsWith("/download/")){return handleDownload(request);}
          if (request->url().startsWith("/delete/")){return handleDelete(request);}
          if (request->url().startsWith("/clear/")){return handleClear(request);}
          if (handleFileRequest(request)) return; //...so, try the FILES handler.
        #endif

        String msg = request->url(); //...assume anything else is command message.
        pending.push(msg);
        request->send(200, "text/plain", "Command queued: "+msg);
        return;

      } //...skip all other api requests.
      request->send(200, "text/plain", "Unknown: "+request->url());
  });

  #ifndef NO_WS
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
  #endif

  server.begin();  // Start the server
}

//-------------------------------------------------------------------------------
#endif