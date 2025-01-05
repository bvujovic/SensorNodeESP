//* ESP8266 initializes module with ENS160 and AHT21 sensors, reads their values and sends them to the hub via ESP-NOW.
//* Current in deep sleep is ~35mA. It might be reduced using ens.setPWRMode().

#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;
#include <DFRobot_ENS160.h>
#define I2C_COMMUNICATION
DFRobot_ENS160_I2C ens(&Wire, /*I2CAddr*/ 0x53);

#include "AirData.h"
AirData airData;

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
// B uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
uint8_t *mac = macSoftEsp32BattConn;
#endif
// void OnDataSent(uint8_t *mac, uint8_t res)
// {
//     Serial.printf("Send status: 0x%X\n", res);
//     // ledOn(sendStatus != 0);
//     if (res != 0)
//         ledOn_10sec();
//     // {
//     //     ledOn(true);
//     //     delay(10 * SEC);
//     // }
//     // ledOn(false);
// }
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
        // ledOn(true);
        // delay(10 * SEC);
        ledOn_10sec();
    }
    // ledOn(false);
    Serial.println("ENS160 found");
    ens.setPWRMode(ENS160_STANDARD_MODE);
    ens.setTempAndHum(26.0, 30.0);

    while (!aht.begin())
    {
        Serial.println("Could not find AHT? Check wiring");
        // ledOn(true);
        // delay(10 * SEC);
        ledOn_10sec();
    }
    // ledOn(false);
    Serial.println("AHT10/AHT20 found");

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        // ledOn(true);
        // while (true)
        //     delay(100);
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

    // ens.setTempAndHum(airData.temperature, airData.humidity);
    ens.setTempAndHum(temp.temperature, hum.relative_humidity);
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
}

void loop()
{
#ifdef USE_ESP_NOW
    auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
    printf("Send res: 0x%X\n", res);
    if (res != 0)
        ledOn_10sec();
    ESP.deepSleep(10 * MIN * 1000);
#else
    ESP.deepSleep(10 * SEC * 1000);
#endif
}
