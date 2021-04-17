# Alexa2ESP
 More than a hundred personalized voice commands per ESP via Alexa voice assistant.

## What is it.
This is a personal project gone public to example a simple way to use the Alexa Voice Assistant, dot or echo, to control an ESP8266 or ESP32. It enables hundreds of voice activated control states for each ESP device, limited only by the maximum allowed Alexa Routines or ESP memory. Alexa to/from ESP communications emulate a dimmable Philips hue dimmable light without the normal on/off switching.

## Code is hacked from Fauxmo library.
Alexa2ESP is poorly hacked version of Xose Pérez's Fauxmo library. The hacks eliminate Alexa's on/off state management to enable the 0 and 100 percent values for use as ESP command values. It also ensures that each Aleax voice command is received by a named Alexa2Esp virtual device (software) only once; an important feature for IR directives and toggel switches at least.

A user provided processing routine (callback method) establishes common command processing using "if-else" or "switch-case" like statements to evaluate the command within the ESP's current state and actions to perform.

## Examples
  1 Minimalist example - simple callback method.
  
  2 Advanced example - extended FIFO command processing method.
  
## Notes
(Now, to just get the code up here too...)
