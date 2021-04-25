
#ifndef CORE_SUPPORT_H
#define CORE_SUPPORT_H


// Alexa voice commands for ESP32 and esp8266

/*-------------------------------------------------------------------------------
ALEXA2ESP ESP
Copyright (C) 2016-2020 by Xose Pérez <xose dot perez at gmail dot com>
The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-------------------------------------------------------------------------------*/

#ifndef ALEXA2ESP_MAXDEVICES
  #define ALEXA2ESP_MAXDEVICES 2
#endif

#define ALEXA2ESP_UDP_MULTICAST_IP     IPAddress(239,255,255,250)
#define ALEXA2ESP_UDP_MULTICAST_PORT   1900
#define ALEXA2ESP_TCP_MAX_CLIENTS      10
//#define ALEXA2ESP_TCP_PORT             80
#define ALEXA2ESP_RX_TIMEOUT           3
#define ALEXA2ESP_DEVICE_UNIQUE_ID_LENGTH  12

#ifndef SERVER_PORT
  #define SERVER_PORT 80
#endif

#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h> //...not async.
#include <queue>
#include <vector>


//-- Globals, Prototypes and such...
std::queue<String> pending;
AsyncWebServer server(SERVER_PORT); // Async server interface
String espName; // The ESP Name based it's IP address (hopefully fixed.)


//=== start Alexa2Esp (fauxmo) ==================================================

//-- templates.h
PROGMEM const char ALEXA2ESP_TCP_HEADERS[] =
  "HTTP/1.1 %s\r\n"
  "Content-Type: %s\r\n"
  "Content-Length: %d\r\n"
  "Connection: close\r\n\r\n";

PROGMEM const char ALEXA2ESP_TCP_STATE_RESPONSE[] = "["
  "{\"success\":{\"/lights/%d/state/off\":%s}},"
  "{\"success\":{\"/lights/%d/state/bri\":%d}}"   // not needed?
"]";

// Working with gen1 and gen3, ON/OFF/%, gen3 requires TCP port 80
PROGMEM const char ALEXA2ESP_DEVICE_JSON_TEMPLATE[] = "{"
  "\"type\": \"Extended color light\","
  "\"name\": \"%s\","
  "\"uniqueid\": \"%s\","
  "\"modelid\": \"LCT015\","
  "\"manufacturername\": \"Philips\","
  "\"productname\": \"E4\","
  "\"state\":{"
    "\"on\": %s,"
    "\"bri\": %d,"
		"\"mode\": \"homeautomation\","
    "\"reachable\": true"
  "},"
  "\"capabilities\": {"
    "\"certified\": false,"
    "\"streaming\": {\"renderer\":true,\"proxy\":false}"
  "},"
  "\"swversion\": \"5.105.0.21169\""
"}";

PROGMEM const char ALEXA2ESP_DESCRIPTION_TEMPLATE[] =
"<?xml version=\"1.0\" ?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion><major>1</major><minor>0</minor></specVersion>"
  "<URLBase>http://%d.%d.%d.%d:%d/</URLBase>"
  "<device>"
    "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
    "<friendlyName>Philips hue (%d.%d.%d.%d:%d)</friendlyName>"
    "<manufacturer>Royal Philips Electronics</manufacturer>"
    "<manufacturerURL>http://www.philips.com</manufacturerURL>"
    "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
    "<modelName>Philips hue bridge 2012</modelName>"
    "<modelNumber>929000226503</modelNumber>"
    "<modelURL>http://www.meethue.com</modelURL>"
    "<serialNumber>%s</serialNumber>"
    "<UDN>uuid:2f402f80-da50-11e1-9b23-%s</UDN>"
    "<presentationURL>index.html</presentationURL>"
  "</device>"
"</root>";

PROGMEM const char ALEXA2ESP_UDP_RESPONSE_TEMPLATE[] =
  "HTTP/1.1 200 OK\r\n"
  "EXT:\r\n"
  "CACHE-CONTROL: max-age=100\r\n" // SSDP_INTERVAL
  "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
  "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n" // _modelName, _modelNumber
  "hue-bridgeid: %s\r\n"
  "ST: urn:schemas-upnp-org:device:basic:1\r\n"  // _deviceType
  "USN: uuid:2f402f80-da50-11e1-9b23-%s::upnp:rootdevice\r\n" // _uuid::_deviceType
  "\r\n";


//-------------------------------------------------------------------------
//-- Convert Alexa value (0-255) to percentage (0-100) and back.
uint8_t pv_arr[] = {0,4,6,9,11,14,16,19,21,24,26,29,31,34,36,39,41,44,47,29,52,54,57,59,62,64,67,69,72,74,77,79,82,84,87,90,92,95,97,100,102,105,107,110,112,115,117,120,122,125,128,130,133,135,138,140,143,145,148,150,153,155,158,160,163,165,168,171,173,176,178,181,183,186,188,191,193,196,198,201,203,206,208,211,214,216,219,221,224,226,229,231,234,236,239,241,244,246,249,251,255};
//-- Use helper methods:
uint16_t p2v(uint16_t p) {return (p>100)?255:pv_arr[p];}
uint16_t v2p(uint16_t v) {uint8_t p=0;for(;p<101&&v>pv_arr[p];p++);return(!p?0:(p<100?p-1:100));}


//-------------------------------------------------------------------------------
//-- fauxmo.h (continued)

typedef struct {
  char* name;
  bool state;
  unsigned char percent;
  unsigned char value;
  char uniqueid[28];
} alexa2espesp_device_t;


#define device_t alexa2espesp_device_t*

typedef std::function<void(device_t)> TSetStateCallback;


class Alexa2Esp {

  public:

    ~Alexa2Esp();

    unsigned char addDevice(const char * device_name, uint8_t pcnt);
    void setDeviceUniqueId(unsigned char id, const char *uniqueid);
    char * getDeviceName(unsigned char id, char * buffer, size_t len);
    int getDeviceId(const char * device_name);

    device_t getDeviceByName(const char * device_name);

    void onSetState(TSetStateCallback fn) { _setCallback = fn; }

    bool process(AsyncClient *client, bool isGet, String url, String body);
    void initialize(void);
    void handle();

    uint8_t setValue(device_t dev, uint8_t val) {
      if(val>0) --val;
      if(val>255) val = 255;
      for (dev->percent=0; dev->percent<101 && val>pv_arr[dev->percent]; dev->percent++);
      dev->value = pv_arr[dev->percent];
      return dev->value;
    }

    void setPercent(device_t dev, uint8_t perc) { // 0-100%
      dev->percent = (perc>100)?100:perc;
      dev->value = pv_arr[dev->percent];
    }

  private:
    AsyncServer * _server;
    unsigned int _tcp_port = SERVER_PORT;
    std::vector<alexa2espesp_device_t> _devices;
    #ifdef ESP8266
      WiFiEventHandler _handler;
    #endif
    WiFiUDP _udp;
    AsyncClient * _tcpClients[ALEXA2ESP_TCP_MAX_CLIENTS];
    TSetStateCallback _setCallback = nullptr;

    String _deviceJson(unsigned char id);

    void _handleUDP();
    void _onUDPData(const IPAddress remoteIP, unsigned int remotePort, void *data, size_t len);
    void _sendUDPResponse();
    void _onTCPClient(AsyncClient *client);
    bool _onTCPData(AsyncClient *client, void *data, size_t len);
    bool _onTCPRequest(AsyncClient *client, bool isGet, String url, String body);
    bool _onTCPDescription(AsyncClient *client, String url, String body);
    bool _onTCPList(AsyncClient *client, String url, String body);
    bool _onTCPControl(AsyncClient *client, String url, String body);
    void _sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime);
};


//-------------------------------------------------------------------------------
//-- fauxmo.cpp

//-- UDP
void Alexa2Esp::_sendUDPResponse() {
  IPAddress ip = WiFi.localIP();
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();

  char response[strlen(ALEXA2ESP_UDP_RESPONSE_TEMPLATE) + 128];
  snprintf_P(
    response, sizeof(response),
    ALEXA2ESP_UDP_RESPONSE_TEMPLATE,
    ip[0], ip[1], ip[2], ip[3],
    _tcp_port,
    mac.c_str(), mac.c_str()
  );

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  #if defined(ESP32)
    _udp.printf(response);
  #else
    _udp.write(response);
  #endif
  _udp.endPacket();
}

void Alexa2Esp::_handleUDP() {
  int len = _udp.parsePacket();
  if (len > 0) {
    unsigned char data[len+1];
    _udp.read(data, len);
    data[len] = 0;

    String request = (const char *) data;
    if (request.indexOf("M-SEARCH") >= 0) {
      if ((request.indexOf("ssdp:discover") > 0) || (request.indexOf("upnp:rootdevice") > 0) || (request.indexOf("device:basic:1") > 0)) {
        _sendUDPResponse();
      }
    }
  }
}

//-- TCP
void Alexa2Esp::_sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime) {
  char headers[strlen_P(ALEXA2ESP_TCP_HEADERS) + 32];
  snprintf_P(
    headers, sizeof(headers),
    ALEXA2ESP_TCP_HEADERS,
    code, mime, strlen(body)
  );
  client->write(headers);
  client->write(body);
}

String Alexa2Esp::_deviceJson(unsigned char id) {
  if (id >= _devices.size()) return "{}";
  alexa2espesp_device_t device = _devices[id];
  char buffer[strlen_P(ALEXA2ESP_DEVICE_JSON_TEMPLATE) + 64];
  snprintf_P(buffer, sizeof(buffer),
    ALEXA2ESP_DEVICE_JSON_TEMPLATE,
    device.name, device.uniqueid,
    device.state ? "true": "false",
		device.value<=1?0:device.value-1
  );
  return String(buffer);
}

bool Alexa2Esp::_onTCPDescription(AsyncClient *client, String url, String body) {
  (void) url;
  (void) body;
  IPAddress ip = WiFi.localIP();
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  char response[strlen_P(ALEXA2ESP_DESCRIPTION_TEMPLATE) + 64];
  snprintf_P(
    response, sizeof(response),
    ALEXA2ESP_DESCRIPTION_TEMPLATE,
    ip[0], ip[1], ip[2], ip[3], _tcp_port,
    ip[0], ip[1], ip[2], ip[3], _tcp_port,
    mac.c_str(), mac.c_str()
  );
  _sendTCPResponse(client, "200 OK", response, "text/xml");
  return true;
}

bool Alexa2Esp::_onTCPList(AsyncClient *client, String url, String body) {
  int pos = url.indexOf("lights"); // Get the index
  if (-1 == pos) return false;
  unsigned char id = url.substring(pos+7).toInt(); // Get the id
	String response;
  if (0 == id) { // Client is requesting all devices
    response += "{";
    for (unsigned char i=0; i< _devices.size(); i++) {
      if (i>0) response += ",";
      response += "\"" + String(i+1) + "\":" + _deviceJson(i);
    }
    response += "}";
  } else {  // Client is requesting a single device
    response = _deviceJson(id-1);
  }
  _sendTCPResponse(client, "200 OK", (char *) response.c_str(), "application/json");
  return true;
}

bool Alexa2Esp::_onTCPControl(AsyncClient *client, String url, String body) {
  if (body.indexOf("devicetype") > 0) { // "devicetype" request
    _sendTCPResponse(client, "200 OK", (char *) "[{\"success\":{\"username\": \"2WLEDHardQrI3WHYTHoMcXHgEspsM8ZZRpSKtBQr\"}}]", "application/json");
    return true;
  }
  if ((url.indexOf("state") > 0) && (body.length() > 0)) {
    uint32_t id = url.substring(url.indexOf("lights")+7).toInt();
    --id; // zero-based for devices array
    if (body.indexOf("bri")>0) { // Brightness value (0-255) from Alexa.
      uint8_t bri = body.substring(body.indexOf("bri") +5).toInt();
      setValue(&_devices[id], bri==1?0:bri); // really should be zero
      if (_setCallback != nullptr) _setCallback(&_devices[id]);
      else pending.push(String(_devices[id].name) + F("/") + String(_devices[id].percent));
    }
		uint8_t rv =  max(0,_devices[id].value-1); // Build response to Alexa.
    char response[strlen_P(ALEXA2ESP_TCP_STATE_RESPONSE)+10];
    snprintf_P(
      response, sizeof(response),
      ALEXA2ESP_TCP_STATE_RESPONSE,
      id+1, rv?"true":"false", id+1, rv
    );
    _sendTCPResponse(client, "200 OK", response, "text/xml");
    return true;
  }
  return false;
}

bool Alexa2Esp::_onTCPRequest(AsyncClient *client, bool isGet, String url, String body) {
  if (url.equals("/description.xml")) {
    return _onTCPDescription(client, url, body);
  }
  if (url.startsWith("/api")) {
    if (isGet) {
      return _onTCPList(client, url, body);
    } else {
         return _onTCPControl(client, url, body);
    }
  }
  return false;
}

bool Alexa2Esp::_onTCPData(AsyncClient *client, void *data, size_t len) {
  char * p = (char *) data;
  p[len] = 0;
  char * method = p; // Method is the first word of the request
  while (*p != ' ') p++;
  *p = 0;
  p++;
  char * url = p;
  while (*p != ' ') p++; // Find next space
  *p = 0;
  p++;
  unsigned char c = 0; // Find double line feed
  while ((*p != 0) && (c < 2)) {
    if (*p != '\r') c = (*p == '\n') ? c + 1 : 0;
    p++;
  }
  char * body = p;
  bool isGet = (strncmp(method, "GET", 3) == 0);
  return _onTCPRequest(client, isGet, url, body);
}

void Alexa2Esp::_onTCPClient(AsyncClient *client) {
  for (unsigned char i = 0; i < ALEXA2ESP_TCP_MAX_CLIENTS; i++) {
    if (!_tcpClients[i] || !_tcpClients[i]->connected()) {
      _tcpClients[i] = client;
      client->onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {}, 0);
      client->onData([this, i](void *s, AsyncClient *c, void *data, size_t len) {
          _onTCPData(c, data, len);
      }, 0);
      client->onDisconnect([this, i](void *s, AsyncClient *c) {
        if (_tcpClients[i] != NULL) {
          _tcpClients[i]->free();
          _tcpClients[i] = NULL;
        }
        delete c;
      },0);
      client->onError([i](void *s, AsyncClient *c, int8_t error) {
        //gsb null for now...
      }, 0);
      client->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
        c->close();
      }, 0);
      client->setRxTimeout(ALEXA2ESP_RX_TIMEOUT);
      return;
    }
  }
  client->onDisconnect([](void *s, AsyncClient *c) {
    c->free();
    delete c;
  });
  client->close(true);
}


//-- Devices
Alexa2Esp::~Alexa2Esp() { } // Permanent, not designed for deconstruction.

void Alexa2Esp::setDeviceUniqueId(unsigned char id, const char *uniqueid) {
  strncpy(_devices[id].uniqueid, uniqueid, ALEXA2ESP_DEVICE_UNIQUE_ID_LENGTH);
}

unsigned char Alexa2Esp::addDevice(const char * device_name, uint8_t pcnt) {
  alexa2espesp_device_t device;
  unsigned int device_id = _devices.size();
  Serial.printf("Init percent 1:  %d\n", pcnt);
  if (pcnt>100) pcnt = 100;
  Serial.printf("Init percent 2:  %d\n", pcnt);

  // init properties
  device.name = strdup(device_name);
  device.state = (!pcnt?false:true);

  // create uniqueid
  String mac = WiFi.macAddress();
  snprintf(device.uniqueid, 27, "%s:%s-%02X", mac.c_str(), "00:00", device_id);
  _devices.push_back(device); // Attach
  setPercent(&device, pcnt); // <-- initial percent w/ implicit vlue
  if (_setCallback != nullptr) _setCallback(&device);
  else // Alternative: 
    pending.push(String(device.name) + F("/") + String(device.percent));
  return device_id;
}

device_t Alexa2Esp::getDeviceByName(const char * device_name) {
  for (unsigned int id=0; id < _devices.size(); id++) {
    if (strcmp(_devices[id].name, device_name) == 0) {
      return &_devices[id];
    }
  }
  return nullptr;
}

int Alexa2Esp::getDeviceId(const char * device_name) {
  for (unsigned int id=0; id < _devices.size(); id++) {
    if (strcmp(_devices[id].name, device_name) == 0) {
      return id;
    }
  }
  return -1;
}

char * Alexa2Esp::getDeviceName(unsigned char id, char * device_name, size_t len) {
  if ((id < _devices.size()) && (device_name != NULL)) {
    strncpy(device_name, _devices[id].name, len);
  }
  return device_name;
}

// Public API
bool Alexa2Esp::process(AsyncClient *client, bool isGet, String url, String body) {
  return _onTCPRequest(client, isGet, url, body);
}

void Alexa2Esp::handle() {_handleUDP();}

void Alexa2Esp::initialize() { // Enable device and start UDP
  ///_enabled = true;
  #ifdef ESP32
    _udp.beginMulticast(ALEXA2ESP_UDP_MULTICAST_IP, ALEXA2ESP_UDP_MULTICAST_PORT);
  #else
    _udp.beginMulticast(WiFi.localIP(), ALEXA2ESP_UDP_MULTICAST_IP, ALEXA2ESP_UDP_MULTICAST_PORT);
  #endif
}

Alexa2Esp alexa2esp;

//=== end alexa2esp ================================================================


//== Optional addin for TP-Link Switches ===========================================
/*  TP-Link - IP Direct Plug Support - OPTIONAL
 *
 *  Updated: 20/01/15
 *
 *  Credits and information here:
 *    https://www.reddit.com/r/tasker/comments/52lvhq/how_to_control_a_tplink_hs100
 *    https://pastebin.com/PtXtKKPW
 *
 *   ON: \x00\x00\x00*\xd0\xf2\x81\xf8\x8b\xff\x9a\xf7\xd5\xef\x94\xb6\xc5\xa0\xd4\x8b\xf9\x9c\xf0\x91\xe8\xb7\xc4\xb0\xd1\xa5\xc0\xe2\xd8\xa3\x81\xf2\x86\xe7\x93\xf6\xd4\xee\xdf\xa2\xdf\xa2
 *  OFF: \x00\x00\x00*\xd0\xf2\x81\xf8\x8b\xff\x9a\xf7\xd5\xef\x94\xb6\xc5\xa0\xd4\x8b\xf9\x9c\xf0\x91\xe8\xb7\xc4\xb0\xd1\xa5\xc0\xe2\xd8\xa3\x81\xf2\x86\xe7\x93\xf6\xd4\xee\xde\xa3\xde\xa3
 *
 *  Tested with TP-Link's HS100, HS103 and HS105. Should
 *  also work with other TP-Link Smart Plugs like HS110.
 *
 *  User List of currently in-use TP-Link Switchs.
 *    switch_t dog_lights = {"HS105","dog lights","192.168.2.141","68:FF:7B:BA:A7:29"};
 *    switch_t night_lights = {"HS100","night lights","192.168.2.109","70:4F:57:75:A2:86"};
 */

//-- Encoded string packet for TP-Link HS100 on/off states.
const char  on_packet[46] = {0x00,0x00,0x00,0x2a,0xd0,0xf2,0x81,0xf8,0x8b,0xff,0x9a,0xf7,0xd5,0xef,0x94,0xb6,0xc5,0xa0,0xd4,0x8b,0xf9,0x9c,0xf0,0x91,0xe8,0xb7,0xc4,0xb0,0xd1,0xa5,0xc0,0xe2,0xd8,0xa3,0x81,0xf2,0x86,0xe7,0x93,0xf6,0xd4,0xee,0xdf,0xa2,0xdf,0xa2};
const char off_packet[46] = {0x00,0x00,0x00,0x2a,0xd0,0xf2,0x81,0xf8,0x8b,0xff,0x9a,0xf7,0xd5,0xef,0x94,0xb6,0xc5,0xa0,0xd4,0x8b,0xf9,0x9c,0xf0,0x91,0xe8,0xb7,0xc4,0xb0,0xd1,0xa5,0xc0,0xe2,0xd8,0xa3,0x81,0xf2,0x86,0xe7,0x93,0xf6,0xd4,0xee,0xde,0xa3,0xde,0xa3};

//-- User Defined TP-Link Switch Structure
typedef struct {
  char * type;
  char * name;
  char * ip;
  char * mac;
} switch_t;

// Set switch mode 'on' or 'off'
void setPlugState(char * ip, bool mode) {  // Change to Switch by name.
  AsyncClient* aClient = new AsyncClient();
  if (aClient->connect(ip, 9999)) {
    aClient->onConnect([&,aClient,mode] (void * arg, AsyncClient* client) {
      if (mode) client->write(on_packet, 46);
      else client->write(off_packet, 46);
      delete client;
    });
  }
}

//==================================================================================
#endif
