//* Current consumption (ESP32, BME680, BSEC, 3.3V): ~45 mA
//* Check current consuption whn esp-now code is enabled and with ESP32 pro mini
// TODO upload this code to ESP32-C3 ProMini, and main_hub to ESP32 narrow board

#include <Arduino.h>
#include "bsec.h" // boschsensortec/BSEC Software Library@^1.8.1492
Bsec iaqSensor;
String output = "";

#include "AirData.h"
AirData airData;

#define SECOND (1000)
#define MINUTE (60 * SECOND)

#include "OneButton.h"
const byte pinBtnSave = 4; // Pin for the button to save settings
OneButton btnSave(pinBtnSave, true);

const byte pinLed = 18;
// const byte pinLed = 3;

void ledOn(bool state)
{
    digitalWrite(pinLed, state ? HIGH : LOW);
}

void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SECOND);
    ledOn(false);
}

void checkIaqSensorStatus()
{
    if (iaqSensor.bsecStatus != BSEC_OK)
    {
        Serial.print("BSEC error code : ");
        Serial.println(iaqSensor.bsecStatus);
    }
    if (iaqSensor.bme68xStatus != BME68X_OK)
    {
        Serial.print("BME680 error code : ");
        Serial.println(iaqSensor.bme68xStatus);
    }
}

#include <EEPROM.h>
#define EEPROM_SIZE 512
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};

void loadState()
{
    if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE)
    {
        Serial.println("Restoring BSEC state...");
        for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
            bsecState[i] = EEPROM.read(i + 1);
        iaqSensor.setState(bsecState);
    }
    else
        Serial.println("No saved state found.");
}

void saveState()
{
    Serial.println("Saving BSEC state...");
    if (iaqSensor.iaqAccuracy >= 2)
    {
        iaqSensor.getState(bsecState);
        EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
        for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
            EEPROM.write(i + 1, bsecState[i]);
        EEPROM.commit();
    }
    else
        Serial.println("IAQ accuracy is too low, not saving state.");
}

#define USE_ESP_NOW
#ifdef USE_ESP_NOW
#include "MacAddresses.h"
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
// B uint8_t *mac = macEsp32BattConn;
uint8_t *mac = macEsp32DevIpex;
esp_now_peer_info_t peerInfo;
// void OnDataSent(const uint8_t *mac, esp_now_send_status_t res)
// {
//     Serial.printf("Send status: 0x%X\n", res);
//     // ledOn(sendStatus != ESP_NOW_SEND_SUCCESS);
// }
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
//- uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
//- uint8_t *mac = macSoftEsp32BattConn;
uint8_t *mac = macSoftEsp32DevIpex;
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

ulong msLastData = 0;

void setup()
{
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    Wire.begin(); // SDA, SCL defaults

    iaqSensor.begin(BME68X_I2C_ADDR_HIGH, Wire);
    checkIaqSensorStatus();

    bsec_virtual_sensor_t sensorList[] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS};

    iaqSensor.updateSubscription(sensorList, sizeof(sensorList) / sizeof(sensorList[0]), BSEC_SAMPLE_RATE_LP);
    //? BSEC_SAMPLE_RATE_ULP_MEASUREMENT_ON_DEMAND
    checkIaqSensorStatus();
    loadState(); // Load previous state if available

    btnSave.attachClick(
        []()
        {
            saveState();
            for (size_t i = 0; i < 3; i++) // Blink LED to indicate save
            {
                ledOn(true);
                delay(500);
                ledOn(false);
                delay(500);
            }
        });

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
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
}

const ulong itvSendData = 10 * MINUTE - 2 * SECOND; // Send data interval

void loop()
{
    btnSave.tick();
    if (iaqSensor.run() && (msLastData == 0 || millis() > msLastData + itvSendData))
    {
        output = String("IAQ: ") + iaqSensor.iaq + " (" + iaqSensor.iaqAccuracy + "), " +
                 "eCO2: " + iaqSensor.co2Equivalent + " ppm, " +
                 "TVOC: " + iaqSensor.breathVocEquivalent + " ppb, " +
                 "Temp: " + iaqSensor.rawTemperature + " °C, " +
                 "Hum: " + iaqSensor.rawHumidity + " %, " +
                 "Pressure: " + (int)(iaqSensor.pressure / 100) + " hPa";
        Serial.println(output);
        Serial.println(String(iaqSensor.temperature) + "\t" + String(iaqSensor.humidity));
#ifdef USE_ESP_NOW
        // B if (cntRetries < maxRetries)
        {
            airData.temperature = iaqSensor.temperature;
            airData.humidity = iaqSensor.humidity + 0.5; // Round to nearest integer
            airData.status = iaqSensor.iaqAccuracy;
            airData.TVOC = iaqSensor.breathVocEquivalent * 100 + 0.5;
            airData.ECO2 = iaqSensor.co2Equivalent + 0.5; // Round to nearest integer
            Serial.println("Sending data via ESP-NOW");
            auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
            printf("Send res: 0x%X\n", res);
            if (res != 0)
                ledOnDelay(10);
        }
        // ESP.deepSleep(10 * (MIN + SEC) * 1000);
        // ESP.deepSleep((10 * (MIN + SEC) - 0.8 * SEC) * 1000); // +0.238sec
        // ESP.deepSleep((10 * (MIN + SEC) - 1 * SEC) * 1000); // +0.238sec
#endif

        ledOn(iaqSensor.iaqAccuracy >= 2); // Turn on LED if IAQ accuracy is good
        msLastData = millis();
    }
    delay(20); // to avoid hammering CPU
}
