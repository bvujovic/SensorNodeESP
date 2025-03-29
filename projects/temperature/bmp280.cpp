//* https://www.esp8266learning.com/esp8266-bmp280-sensor-example.php

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;

#define SEC (1000)
#define MIN (60 * SEC)

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOn_10sec()
{
    ledOn(true);
    delay(10 * SEC);
    ledOn(false);
}

#define USE_ESP_NOW
#include "MacAddresses.h"

#ifdef USE_ESP_NOW
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
// B uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x04};
uint8_t *mac = macEsp32BattConn;
esp_now_peer_info_t peerInfo;
// void OnDataSent(const uint8_t *mac, esp_now_send_status_t res)
// {
//     Serial.printf("Send status: 0x%X\n", res);
//     // ledOn(sendStatus != ESP_NOW_SEND_SUCCESS);
// }
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
uint8_t *mac = macSoftEsp32BattConn;
#endif
#endif

void setup()
{
    Serial.begin(115200);
    Serial.println("\nBMP280 test");
    pinMode(pinLed, OUTPUT);
    ledOn(false);

    // uint status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
    // uint status = bmp.begin();
    while (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID))
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                         "try a different address!"));
        Serial.print("SensorID was: 0x");
        Serial.println(bmp.sensorID(), 16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        ledOn_10sec();
    }

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        ledOn_10sec();
    }
#ifdef ESP32
    // esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        // digitalWrite(pinLed, false);
        ledOn(true);
        while (true)
            delay(100);
    }
#else
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    // esp_now_register_send_cb(OnDataSent);
    auto res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    printf("esp_now_add_peer res: 0x%X\n", res);
    if (res != 0)
        ledOn_10sec();
#endif
#endif
}

void loop()
{
    // Serial.print("Temperature = ");
    // Serial.print(bmp.readTemperature());
    // Serial.println(" *C");

    auto t = bmp.readTemperature();
    Serial.println(t);
#ifdef USE_ESP_NOW
    auto res = esp_now_send(mac, (uint8_t *)&t, sizeof(t));
    printf("Send res: 0x%X\n", res);
    if (res != 0)
        ledOn_10sec();
    ESP.deepSleep(10 * (MIN + SEC) * 1000);
#else
    ESP.deepSleep(10 * SEC * 1000);
#endif


    // Serial.print("Pressure = ");
    // Serial.print(bmp.readPressure());
    // Serial.println(" Pa");
    // Serial.print("Approx altitude = ");
    // Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forcase
    // Serial.println(" m");
    // Serial.println();
    delay(10000);
}
