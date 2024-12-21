#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;
#include <DFRobot_ENS160.h>
#define I2C_COMMUNICATION
DFRobot_ENS160_I2C ens(&Wire, /*I2CAddr*/ 0x53);

#include "AirData.h"
AirData airData;

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }

#define SEC (1000)
#define MIN (60 * SEC)

#define USE_ESP_NOW

#ifdef USE_ESP_NOW
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x04};
esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus) { ledOn(sendStatus != ESP_NOW_SEND_SUCCESS); }
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
#endif
void OnDataSent(uint8_t *mac, uint8_t sendStatus) { ledOn(sendStatus != 0); }
#endif

void setup()
{
    Serial.begin(115200);
    Serial.println();
    pinMode(pinLed, OUTPUT);
    ledOn(false);

    while (NO_ERR != ens.begin())
    {
        Serial.println("Communication with ENS160 device failed, please check connection");
        ledOn(true);
        delay(10 * SEC);
    }
    ledOn(false);
    Serial.println("ENS160 found");
    ens.setPWRMode(ENS160_STANDARD_MODE);
    ens.setTempAndHum(26.0, 30.0);

    while (!aht.begin())
    {
        Serial.println("Could not find AHT? Check wiring");
        ledOn(true);
        delay(10 * SEC);
    }
    ledOn(false);
    Serial.println("AHT10/AHT20 found");

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        ledOn(true);
        while (true)
            delay(100);
    }
#ifdef ESP32
    esp_now_register_send_cb(OnDataSent);
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
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
#endif
#endif
}

void loop()
{
    // AHT21
    sensors_event_t hum, temp;
    aht.getEvent(&hum, &temp);
    airData.temperature = (int)(temp.temperature + 0.5);
    airData.humidity = (int)(hum.relative_humidity + 0.5);
    Serial.print("Temp: ");
    Serial.print(airData.temperature);
    Serial.print(", Hum: ");
    Serial.print(airData.humidity);
    Serial.println("% rH");
    ens.setTempAndHum(airData.temperature, airData.humidity);

    // ENS160
    airData.status = ens.getENS160Status();
    airData.AQI = ens.getAQI();
    airData.TVOC = ens.getTVOC();
    airData.ECO2 = ens.getECO2();
    Serial.print("ENS160 status: ");
    Serial.print(airData.status);
    Serial.print(",  CO2 equivalent: ");
    Serial.print(airData.ECO2);
    Serial.print(" ppm, TVOC: ");
    Serial.print(airData.TVOC);
    Serial.print(", AQI: ");
    Serial.println(airData.AQI);

#ifdef USE_ESP_NOW
    esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
    delay(10 * MIN);
#else
    delay(10 * SEC);
#endif
}
