// Complete Instructions to Get and Change ESP MAC Address: https://RandomNerdTutorials.com/get-change-esp32-esp8266-mac-address-arduino/
// To communicate via ESP-NOW, you need to know the MAC Address of the ESP8266 receiver.
// My NodeMCU's MAC address: 84:F3:EB:77:04:BA
// My Wemos with external antenna - MAC address: 40:F5:20:3E:D5:11, current: 73mA, 5V
// ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04
// ESP32 WROOM-32 test device (yellow pins) MAC Address: 30:AE:A4:47:9C:C4
// ESP-32S, micro USB, narrow, built-in antenna MAC Address: 78:1C:3C:CA:FE:38
// ESP32-C3 ProMini 1 MAC Address: 24:EC:4A:C8:A7:2C

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    Serial.print("\nESP Board MAC Address: " + WiFi.macAddress());
    delay(2000);
}
