//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

#include <SrxParser.h>
SrxParser srx;

const byte pinRadioIn = 19;
const byte pinLed = 22; // LED_BUILTIN
const byte pinBuzz = 23;
bool isDataReceived = false;

char msg[10];

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
    // Serial.println("Data received!");
    // Serial.println(len);
    // Serial.println((char *)incomingData);
    isDataReceived = true;
    memcpy(msg, incomingData, len);
    Serial.println(atoi(msg));
}

void setup()
{
    pinMode(pinRadioIn, INPUT);
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, true);
    pinMode(pinBuzz, OUTPUT);
    digitalWrite(pinBuzz, false);
    Serial.begin(115200);
    Serial.println("\n *** SensorNodeESP: HUB ***");
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (true)
            delay(100);
    }
#ifdef ESP32
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
#else
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
#endif
}

void loop()
{
    
    if (isDataReceived)
    {
        isDataReceived = false;
    }

    // Parsing signals from simple sensors (PIR, water detection...) with SRX882
    SrxCommand cmd = srx.refresh(pulseIn(pinRadioIn, LOW), millis());
    if (cmd != None)
    {
        Serial.printf("SRX882: %d\n", cmd);
    }
}
