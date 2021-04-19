# Alexa2ESP
 More than a hundred personalized voice commands per ESP via Alexa voice assistant.

## What is it.
This is a personal project gone public to example a simple way to use the Alexa Voice Assistant, dot or echo, to control an ESP8266 or ESP32. It enables hundreds of project taylored voice activated control states for each ESP device, limited only by the maximum allowed Alexa Routines. Alexa to/from ESP communications emulate a dimmable Philips hue light without the normal on/off switching.

## Code is hacked from Fauxmo library.
Alexa2ESP is poorly hacked version of Xose Pérez's Fauxmo library. The hacks eliminate Alexa's on/off state management to enable the 0 and 100 percent values for use as ESP command values. It also ensures that each Aleax voice command is received by a named Alexa2Esp virtual device (software) only once; an important feature for IR directives and toggel switches at least.

A user provided processing routine (callback method) establishes common command processing using "if-else" or "switch-case" like statements to evaluate the command within the ESP's current state and actions to perform.

## Examples
  1. Minimalist example - simple callback method
     - [PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu
     - Alexa2Esp enabling 2 virtual devices, both w/ 101 possible voice commands
     - Callback method for command processing
     - Async Web Server

  2. Advanced example - FIFO command queue and processing method
     - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu
     - Alexa2Esp enabling 2 virtual devices, both w/ 101 possible voice commands
     - FIFO pending queue and command processing
     - Async Web Server
     - Websockets and Web Monitor with Input Field for commands
     - File system with send-file, upload, download, delete and clear
     - OTA for both sketch and data
     - Local Time and Date management
     - Timers for interval and one-up function calls
     - TP-Link Switch controls
