//* ESP8266 initializes module with ENS160 and DHT22 sensors, reads their values and sends them to the hub via ESP-NOW.
//? Current in deep sleep is ~35mA. It might be reduced using ens.setPWRMode().

#include <DHT.h>      // lib_deps = adafruit/DHT sensor library@^1.4.6
#define DHTPIN D5     // DHT sensor pin on ESP8266 D1 Mini Lite
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#include <DFRobot_ENS160.h>
#define I2C_COMMUNICATION
DFRobot_ENS160_I2C ens(&Wire, /*I2CAddr*/ 0x53);

#include "AirData.h"
AirData airData;

#define SEC (1000)
#define MIN (60 * SEC)

const byte maxRetries = 3; // Max retries for getting data from sensors
byte cntRetries = 0;       // Counter for retries
bool shouldRetry = false;  // Flag to indicate if we should retry reading sensors

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SEC);
    ledOn(false);
}

#define USE_ESP_NOW
#ifdef USE_ESP_NOW
#include "MacAddresses.h"
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

    dht.begin();

    while (NO_ERR != ens.begin())
    {
        Serial.println("Communication with ENS160 device failed, please check connection");
        // ledOn(true);
        // delay(10 * SEC);
        ledOnDelay(10);
    }
    // ledOn(false);
    Serial.println("ENS160 found");
    ens.setPWRMode(ENS160_STANDARD_MODE);
    ens.setTempAndHum(25.0, 50.0); // Set default temperature and humidity

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        // ledOn(true);
        // while (true)
        //     delay(100);
        ledOnDelay(10);
    }
    // if (esp_now_init() != 0)
    // {
    //     Serial.println("ESP NOW INIT FAIL");
    //     // ledOn_10sec();
    //     cntRetries++;
    // }

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
        ledOnDelay(10);
    // cntRetries++;
#endif
#endif
    do
    {
        // DHT22
        float hum = dht.readHumidity();
        float temp = dht.readTemperature();
        if (isnan(hum) || isnan(temp))
        {
            Serial.println("Failed to read from DHT sensor!");
            ens.setTempAndHum(25.0, 50.0); // Set default temperature and humidity
            airData.temperature = 0;
            airData.humidity = 0;
            shouldRetry = true;
        }
        if (hum > 100)
            shouldRetry = true;
        airData.temperature = temp;
        airData.humidity = (int)(hum + 0.5); // Round to nearest integer
        Serial.print("Temp: ");
        Serial.print(airData.temperature);
        Serial.print(", Hum: ");
        Serial.print(airData.humidity);
        Serial.println("% rH");

        ens.setTempAndHum(temp, hum); // Set temperature and humidity for ENS160
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
        if (airData.AQI == 0 || airData.status == 1)
            shouldRetry = true;
        if (shouldRetry)
        {
            cntRetries++;
            Serial.printf("Retrying... (%d/%d)\n", cntRetries, maxRetries);
            ledOnDelay(10);
            if (airData.status == 1)
                ledOnDelay(60); // If in warm-up phase, wait longer
        }
        // else
        //     cntRetries = 0;
    } while (shouldRetry && cntRetries < maxRetries);

#ifdef USE_ESP_NOW
    if (cntRetries < maxRetries)
    {
        Serial.println("Sending data via ESP-NOW");
        res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
        printf("Send res: 0x%X\n", res);
        if (res != 0)
            ledOnDelay(10);
    }
    // ESP.deepSleep(10 * (MIN + SEC) * 1000);
    // ESP.deepSleep((10 * (MIN + SEC) - 0.8 * SEC) * 1000); // +0.238sec
    ESP.deepSleep((10 * (MIN + SEC) - 1 * SEC) * 1000); // +0.238sec
#else
    ESP.deepSleep(10 * SEC * 1000);
#endif
}

void loop()
{
}
