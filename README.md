# SensorNodeESP

Data from sensor (PIR, CO2, smoke/water detectors...) is logged and sometimes sent to user (WhatsApp message, E-mail).

## Server (Hub): ESP32 w/ buzzer
![Server: ESP32 (SRX882, buzzer)](projects/hub/docs/esp32_hub_device.jpg)

## Web app - interface
![Web app - interface](projects/hub/docs/web_page_interface.png)

## Windows desktop app: backup logs (data from sensors)
![Windows desktop app](_WinBackupApp/WinBackupApp.png)

## Air Quality Data
ESP wakes up every 10 minutes and sends data from sensors to the hub (server) via ESP-NOW.

### SCD30 - CO2
![Client: SCD30 & CO2](projects/_clients/scd30/wemos_scd30.jpg)

### ENS160, DHT22 - air quality data (temp, hum, TVOC)
![Client: ESP8266 & ENS160+AHT21](projects/_clients/ens160_dht22/docs/wemos_ens160_dht22_client_finished.jpg)

## Client: ESP32, Li-Ion 18650 battery, ESP-NOW
ESP32 device wakes on a pin event (e.g. wires are submerged, PIR signals HIGH...), sends an ESP-NOW message to a predefined MAC address (hub) and then goes back to deep sleep.
### Water detection
![ESP32, Li-Ion 18650 battery, water detection wires](projects/_clients/esp32-wake-on-pin/docs/esp32-wake-on-pin_device.jpg)

## TODO
- [ ] (WIP) Adjust WinBackupApp so it can run @Vranic
- [x] Pay subscription to the CallMeBot
- [ ] Hub:
  - [ ] (OPT) PIR on Hub: add PIR sensor directly to the Hub so that another esp32-wake-on-pin client isn't necessary for movement detection. Hub will also log and send a message to the user (WhatsApp).
  - [ ] Make 2nd version of Hub project - Hub /wo internet for places without internet access or with unknown net credentials. Communication with clients is done via ESP-NOW or radio (HC-12, LoRa, http...). Maybe it could have its own wireless network for web app access?
  - [ ] Web App:
      - [ ] Improve interface (chart.js disappears, shrinks)
  - [ ] Messages to the hub: ESP32CAM - take a picture, indoor vehicle - start, move...
    - [ ] (WIP) Make ESP32-C3 client that sleeps and gets a message from the hub
    - [x] Why doesn't ESP32-C3 XIAO work with PIR sensor 
    - [ ] Test current hub code @BanovoBrdo
    - [ ] Put Azure code in a class (e.g. AzureMessages/AzureThings)
    - [ ] ESP32-C3 turn up ESP32CAM and forward message from the hub
- [ ] Clients:
  - [ ] ClientLogger
      - [ ] Use it in SCD30 and ENS&DHT projects
  - [ ] Put retrying logic in a class: Retryer
  - [ ] TimeSlotSend
      - [ ] Change getDeepSleepTime() so that it takes into account wake time and time spent in sending data
  - [ ] (OPT) Put TSS, LoggerMin and retrying logic in a class that will be used by most clients that report data regularly to the hub
  - [ ] SCD30:
      - [ ] Button click: print log on Serial and send data (prev data or wait for new?) to the hub
  - [x] esp32-wake-on-pin: new version that supports PIR w/ transistor
  - [ ] Add more sensor nodes
    - [ ] (WIP) SCD41 (CO2 sensor)
    - [ ] Microphone (noise levels)

## Add new client routine
- my_esp_now.h:
    - setPeers(): add new setPeer() call
        - setPeer(): set parameters: mac address, sensor type, device
    - Increase length of peers array: peer_info peers[]
    - OnDataRecv(): if (p->type == SensorType::...)
- lib/Common/src/ToString.cpp:
    - Add new members to arrays: StrSensorTypes[], SensorTypesComment[]
- Enums.h: Add new members to SensorType, Device
- index.html:
    - CmbChartParamsChange(): adjusting chart for sensors with temp and hum
    - lastChartParam: add default property for new sensor

## Links
