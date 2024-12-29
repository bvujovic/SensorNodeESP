# SensorNodeESP

Data from sensor (PIR, CO2, smoke/water detectors...) is logged and sometimes sent to user (WhatsApp message, E-mail).

ATtiny sleeps, wakes up on HIGH (PIR...), sends signal via STX882, goes back to sleep. Device can be battery powered. 

## Sender: ATtiny85, STX882, PIR - test device
![ATtiny85, STX882 - test device](projects/attiny_stx882/docs/attiny_stx882_test_device_pir.jpg)

## Receiver: ESP8266, SRX882 - test device
![ESP8266, SRX882 - test device](projects/attiny_stx882/docs/esp8266_srx882_test_device.jpg)

## TODO

- [x] ATtiny sleep, wake up on button, PIR...
- [x] ATtiny sends signal via STX882, hub receives that signal
- [x] ESP8266 sending message via ESP Now or HTTP web request to another ESP (8266/32).
- [ ] Hub - central component: ESP32 receives and logs messages (source/sensor, type:info/warning/error/alarm..., message/data ...) and then sends WA message or e-mail.

## Links
https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/
https://randomnerdtutorials.com/esp-now-two-way-communication-esp8266-nodemcu/
https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
https://randomnerdtutorials.com/esp32-esp-now-wi-fi-web-server/
https://rntlab.com/question/esp-now-gateway-wifi_mode_sta-with-a-wifi-router/
https://forum.arduino.cc/t/use-esp-now-and-wifi-simultaneously-on-esp32/1034555/16

## Remarks
 - Works if WiFi channel on the router is set to 1. There might be ways to fix that by using wifi_promiscuous_enable()...

