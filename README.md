# SensorNodeESP

Data from sensor (PIR, CO2, smoke/water detectors...) is logged and sometimes sent to user (WhatsApp message, E-mail).

## Server (Hub): ESP32 (SRX882, buzzer)
![Server: ESP32 (SRX882, buzzer)](projects/hub/docs/esp32_server_device.jpg)

## Web app - interface
![Web app - interface](projects/hub/docs/web_page_interface.png)

## Client: ESP8266 & ENS160+AHT21
ESP wakes up every 10 minutes and sends data from sensors to the hub (server) via ESP-NOW.
![Client: ESP8266 & ENS160+AHT21](projects/ens160_aht21/docs/nodemcu_ens160_aht21.jpg)

## Client: ATtiny85 & STX882
ATtiny sleeps, wakes up on HIGH (test button, PIR, wires for water detection...), sends signal via STX882, goes back to sleep. Device can be battery powered.
### Movement detection
![Client: ATtiny85 (STX882...)](projects/attiny_stx882/docs/attiny_stx882_test_device_pir.jpg)
### Water detection
![Client: ATtiny85 (STX882...)](projects/attiny_stx882/docs/attiny_stx882_finished_water_detect.jpg)

## TODO
- [ ] README.md: present server first and clients later. Change images where necessary.
- [ ] Add more sensor nodes
    - [ ] BME680
        - [x] Create sketch that sends data to the hub
        - [ ] Use BSEC2 library and ESP32 C3 Pro Mini for the same functionality
        - [ ] Improve current consumption
    - [ ] SCD30
        - [x] Create test sketch that prints values on serial monitor
        - [ ] Add code for calibration: fresh air once a week
            - [ ] When sketch starts, if btn pressed - FRC, else - normal operation. Maybe One button is not needed.
        - [ ] Add ESP-NOW communication: send data to the hub every 10 minutes
        - [ ] Improve current consumption: ESP deep sleep, variable measurement interval for SCD30
    - [ ] ATtiny & HC-12
        - [ ] Is ESP32 hub impaired by calling HC12.available()? Does it interfere with buzzer?
        - [ ] Remove ATtiny & STX882 from project if STX882 can't work precisely with ATtiny85 as with other microcontrollers
- [ ] Improve web app interface
- [ ] Add 5V buzzer (with transistor)
- [ ] Check if it's possible for ESP8266 to get back a value from the hub (two-way communication) and then go to sleep.
    - [ ] If that's possible, write a code that returns how much (milli)seconds should client wait until next reporting to the hub.

## Add new client routine
- my_esp_now.h: in setPeers() add new setPeer() call
    - setPeer(): set parameters: mac address, sensor type, device
- Increase length of peers array: peer_info peers[]
- Add new members to arrays: StrSensorTypes[], SensorTypesComment[]
- HTML file:...
...

## Links

### ESP-NOW
- https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/
- https://randomnerdtutorials.com/esp-now-two-way-communication-esp8266-nodemcu/
- https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
- https://randomnerdtutorials.com/esp32-esp-now-wi-fi-web-server/
- https://rntlab.com/question/esp-now-gateway-wifi_mode_sta-with-a-wifi-router/
- https://forum.arduino.cc/t/use-esp-now-and-wifi-simultaneously-on-esp32/1034555/16

## Remarks
 - ESP-NOW communication works if WiFi channel on the router is set to 1. There might be ways to fix that by using wifi_promiscuous_enable()...

