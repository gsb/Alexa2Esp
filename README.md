# Alexa2ESP
 Ninty-nine personalized voice commands via the Alexa voice assistant for your ESPs, and 99 is a lot!

## What is it.
This is a personal project gone public to example a simple way to use the Alexa Voice Assistant, dot or echo, to control an ESP8266 or ESP32. It enables ninty-nine project taylored voice activated control states for ESP devices. Alexa to/from ESP communications emulate a dimmable Philips hue light without the normal on/off switching.

## Code is hacked from Fauxmo library.
Alexa2Esp is poorly hacked version of Xose Pérez's Fauxmo library, IMHO. The hacks eliminate Alexa's on/off state management to  enable the 0 and 100 percent values for use as ESP command values. It also ensures that each Aleax voice command is received only once; an important feature for IR directives and toggel switches.

A user provided processing routine (FIFO loopback queueing method) establishes common command processing using "if-else" and/or "switch-case" like statements to validate and process the command within the ESP's current state.

## Examples
  1. Minimalist example - simple callback method
     - [PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu
     - Alexa2Esp enabling a virtual device with ninty-nine possible voice commands
     - Loopback (FIFO queue) method for command processing
     - Async Web Server

  2. Advanced example - FIFO command queue and processing method
     - PlatformIO w/ Arduino framework for both ESP32s and ESP8266 nodemcu
     - Alexa2Esp enables a virtual device with ninty-nine possible voice commands
     - Loopback (FIFO queue) method for command processing
     - Async Web Server
     **Options to exclude following features, default, included.
     - Websockets and Web Monitor with Input Field for commands
     - File system with send-file, upload, download, delete and clear
     - OTA for both sketch and data
     - Local Time and Date management
     - Timers for interval and one-up function calls
     - TP-Link Switch controls (provided in support.h)
