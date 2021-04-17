# Alexa2ESP
 More than a hundred personalized voice commands per ESP via Alexa voice assistant.

## What is it.
This is a personal project gone public to example a simple way to use the Alexa Voice Assistant, dot or echo, to control an ESP, ESP8266 or ESP32. It enables hundreds of voice activated control states for each ESP device, limited only by the maximum allowed Alexa Routines. Alexa to/from ESP communications emulate a dimmable Philips hue Light without its normal on/off switching.

## Code is hacked from EspAlexa library.
Alexa2ESP is poorly hacked version of Christian Schwinne's EspAlexa library - https://github.com/Aircoookie/Espalexa. The hacks eliminate Alexa's on/off state management, color management and enables the 0 and 100 percent values for use as ESP state values. It ensures that each Aleax voice command is received by a named alexa2esp-device (software) only once; an important feature.

## Examples
  1 Minimalist example.
  2 Exposed async-server enabling HTTP-GET requests for state changes.
  3 Introduces formatted command messaging: target/value.
  4 Introduces FIFO queued command processing, good for async server.
    (Comptational intensive processes triggered by a state change should be isolated to avoid async-server timing problems.)

## Usage
(See examples for now.)
