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
- [ ] README.md: Change images where necessary.
- [ ] Hub:
    - [ ] Split Hub project into 2:
        - Hub: connected to the internet, communication with clients is done via ESP-NOW. First, create alternative version for attiny_stx882 project (Water detection).
        - Hub /wo internet: communication with clients is done via radio (HC-12, ~~SRX882~~...). Maybe it could have its own wireless network for web app access?
            - Test if it's possible for 2 ESP32's to communicate /wo (known) WiFi network. If it's possible, create project with 2 simple parts: client (reacts on some simple event e.g. PIR) and server (beeps when ESP-NOW msg from client is received)
    - [ ] Web App:
        - [ ] Improve interface (chart.js disappears, shrinks)
    - [ ] Add 5V buzzer (with transistor)
- [ ] Clients:
    - [ ] SCD30:
        - [ ] Check if pinButton is pressed before wait for response from airSensor (SCD30)
        - [ ] Check documantation for setting the right temperature \[offset\]
        - [ ] Try to use TimeSlotSend class 
    - [ ] (WIP) TimeSlotSend class - solution for sleep and timely reporting by clients: Client will sleep for e.g. 9 minutes, and then asks hub for current time and then sends data based on that time.
        - [x] Use this class in ens160_dht22 project.
    - [ ] Add more sensor nodes
        - [ ] Replace attiny_stx882 with another device/project: ESP32 w/ battery connector that uses ESP-NOW for communication with the hub. Basis for this projects' code is in spikes/client-sleep_wake-on-pin.cpp
            - [x] Test if device detects water after longer time of inactivity (1-10h hours after it's turned on).
            - [ ] Resend message couple of seconds after the first one.
            - [ ] Test if device works with PIR sensor
            - [ ] Create project folder: name, code, pictures, README.md
        - [ ] ATtiny & HC-12
            - [ ] Is ESP32 hub impaired by calling HC12.available()? Does it interfere with buzzer?
            - [ ] Remove ATtiny & STX882 from project if STX882 can't work precisely with ATtiny85 as with other microcontrollers
        - [ ] Microphone (noise levels)

## Add new client routine
- my_esp_now.h:
    - setPeers(): add new setPeer() call
        - setPeer(): set parameters: mac address, sensor type, device
    - Increase length of peers array: peer_info peers[]
    - Add new members to arrays: StrSensorTypes[], SensorTypesComment[]
    - OnDataRecv(): if (p->type == SensorType::...)
- Enums.h: Add new members to SensorType, Device
- index.html:
    - CmbChartParamsChange(): adjusting chart for sensors with temp and hum
    - lastChartParam: add default property for new sensor

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

