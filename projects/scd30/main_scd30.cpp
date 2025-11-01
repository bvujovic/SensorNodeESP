//* ESP8266, SCD30, TM1637, button
//* https://sensirion.com/products/catalog/SCD30

#include "Arduino.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" // sparkfun/SparkFun SCD30 Arduino Library@^1.0.20
SCD30 airSensor;

#include "AirData.h"
AirData airData;
#define SECOND (1000)
#define MINUTE (60 * SECOND)

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SECOND);
    ledOn(false);
}
void ledOnForever()
{
    ledOn(true);
    while (true)
        delay(1000);
}
void ledOnRestart()
{
    ledOnDelay(10);
    ESP.restart();
}

#define USE_ESP_NOW
#ifdef USE_ESP_NOW
#include "MacAddresses.h"
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
uint8_t *mac = macEsp32DevIpex;
esp_now_peer_info_t peerInfo;
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
uint8_t *mac = macSoftEsp32DevIpex;
#endif
#endif

int prevCO2 = 0;                       // Previous CO2 value
float prevTemp = 0.0f, prevHum = 0.0f; // Previous temperature and humidity values

#include "OneButton.h" // mathertel/OneButton@^2.0.0
const byte pinButton = D6;
OneButton btn(pinButton, true); // Button to change display mode

void printValues(uint16_t co2, float temp, float hum)
{
    Serial.print("Displaying CO2: ");
    Serial.print(co2);
    Serial.print(" ppm, Temp: ");
    Serial.print(temp, 1);
    Serial.print(" C, Humidity: ");
    Serial.print(hum, 1);
    Serial.println(" %");
}

ulong msStartFrcCD = 0;

void setup()
{
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    pinMode(pinButton, INPUT_PULLUP);
    Serial.begin(115200);
    Wire.begin();
    while (!airSensor.begin())
    {
        Serial.println("Air sensor not detected. Please check wiring. Freezing...");
        ledOnRestart();
    }
    airSensor.setAutoSelfCalibration(false);
    Serial.print("Auto calibration set to: ");
    Serial.println(airSensor.getAutoSelfCalibration());
    airSensor.setAltitudeCompensation(170); // Set altitude compensation to 170m
    Serial.print("Altitude compensation set to: ");
    Serial.println(airSensor.getAltitudeCompensation());
    airSensor.setMeasurementInterval(60);
    Serial.print("Measurement interval set to: ");
    Serial.println(airSensor.getMeasurementInterval());

#ifdef USE_ESP_NOW
    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        ledOnRestart();
    }
#ifdef ESP32
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        ledOnRestart();
    }
#else
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    auto res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    printf("esp_now_add_peer res: 0x%X\n", res);
    if (res != 0)
        ledOnRestart();
#endif
#endif

    // btn.attachClick(
    //     []()
    //     {
    //         Serial.println("Button clicked, changing display mode...");
    //         displayMode = (DisplayMode)((int(displayMode) + 1) % int(MaxModes)); // Cycle through display modes
    //         printValues(prevCO2, prevTemp, prevHum);                            // Show previous CO2, temperature and humidity
    //     });
    btn.attachLongPressStart(
        []()
        {
            Serial.println("Button long pressed, toggling measurement interval...");
            airSensor.setMeasurementInterval(airSensor.getMeasurementInterval() == 60 ? 5 : 60); // Toggle measurement interval
            // display.showNumberDec(airSensor.getMeasurementInterval());                           // Show measurement interval on the display
            Serial.print("Measurement interval set to: ");
            Serial.println(airSensor.getMeasurementInterval());
            delay(1000);                             // Wait for a second to let the user read the display
            printValues(prevCO2, prevTemp, prevHum); // Show previous CO2, temperature and humidity
        });
    btn.attachDoubleClick(
        []()
        {
            // delay(60000); // Wait for 60 seconds to let the sensor stabilize
            msStartFrcCD = millis(); // Start time for forced calibration
            Serial.println("Button double clicked, starting forced recalibration with 425 ppm reference in 20 minutes.");
            // display.showNumberDecEx(0, 0b01000000, true); // Show 00:00 on the display to indicate forced recalibration
            // airSensor.setForcedRecalibrationFactor(425); // Set forced recalibration factor to 425 ppm
            // airSensor.setAutoSelfCalibration(false); // Optional: turn ASC off if you don't ventilate regularly
            // Serial.println("Button double clicked, forced recalibration with 400 ppm reference.");
            // display.showNumberDec(425); // Show 425 on the display to indicate forced recalibration
            delay(1000); // Wait for a second to let the user read the display
            airSensor.setMeasurementInterval(60);
        });
}

ulong ms = 0; // Variable to store the last time the sensor was checked

void loop()
{
    if (millis() > ms + 1000)
    {
        if (airSensor.dataAvailable())
        {
            uint16_t co2 = airSensor.getCO2();
            float temp = airSensor.getTemperature();
            float hum = airSensor.getHumidity();
            if (msStartFrcCD > 0)
            {
                Serial.println("Forced recalibration in progress...");
                // display.showNumberDec(1111);
                delay(1000); // Show 1111 on the display to indicate forced recalibration in progress
            }
            printValues(co2, temp, hum);
#ifdef USE_ESP_NOW
            if (co2 == 0)
                Serial.println("CO2 is zero, skipping ESP-NOW send.");
            {
                Serial.println("Sending data via ESP-NOW");
                airData.ECO2 = co2;
                airData.temperature = temp;
                airData.humidity = hum + 0.5f; // Round humidity
                auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
                printf("Send res: 0x%X\n", res);
                if (res != 0)
                    ledOnDelay(10);
                // ESP.deepSleep(10 * (MIN + SEC) * 1000);
            }
#endif
            prevCO2 = co2;
            prevTemp = temp;
            prevHum = hum;
        }
        ms = millis();
    }

    if (msStartFrcCD > 0 && millis() >= msStartFrcCD + 20UL * 60 * 1000) // 20 minutes passed
    {
        msStartFrcCD = 0;                            // Reset the start time
        airSensor.setForcedRecalibrationFactor(425); // Set forced recalibration factor to 425 ppm
        airSensor.setAutoSelfCalibration(false);     // Optional: turn ASC off if you don't ventilate regularly
        Serial.println("Forced recalibration with 425 ppm reference completed.");
        // display.showNumberDec(425); // Show 425 on the display to indicate forced recalibration
        delay(1000); // Wait for a second to let the user read the display
    }

    btn.tick();
    delay(10);
}
