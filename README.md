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
    - [ ] Web App:
        - [ ] Display cmbSensor value on btnRefresh click so if the web app didn't load completely - click on that button would load web app properly and not only sensor data
        - [ ] ? Add https://xeco.info/xeco/vazduh/Beograd%20%C4%8Cukarica-Beogradskog%20Bataljona-1100127923 to Links section (location is not that near me :\ )
        - [ ] Improve interface (chart.js disappears, shrinks)
    - [ ] Add 5V buzzer (with transistor)
- [ ] Clients:
    - [ ] SCD30:
        - [ ] Check if pinButton is pressed before wait for response from airSensor (SCD30)
        - [ ] Try to use TimeSlotSend class 
    - [ ] (WIP) TimeSlotSend class - solution for sleep and timely reporting by clients: Client will sleep for e.g. 9 minutes, and then asks hub for current time and then sends data based on that time.
        - [ ] Use this class in ens160_dht22 project.
        - [ ] Should (re)trying to send data (when something is wrong) be a part of this class?
    - [ ] Add more sensor nodes
        - [ ] (WIP) Test current consumption in deep sleep and time it takes for ESP32 module to send msg via ESP-NOW for: ESP32-C3 Pro Mini and ESP32 w/ battery connector. Can those modules be used for PIR/water detection? Try detecting water with wires and ESP32 w/ battery connector.
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

