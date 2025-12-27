//* Device reports CO2, temperature and humidity from SCD30 sensor via ESP-NOW to a receiver device (hub).
//* ESP8266, SCD30, button
//* https://sensirion.com/products/catalog/SCD30

#include "Arduino.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" // sparkfun/SparkFun SCD30 Arduino Library@^1.0.20
SCD30 airSensor;

#include "AirData.h"
AirData airData;
#define SECOND (1000UL)
#define MINUTE (60 * SECOND)

// https://sensirion.com/media/documents/0FEA2450/61652EF9/Sensirion_CO2_Sensors_SCD30_Low_Power_Mode.pdf
#define ITV_FRC (10 * 60)      // Forced Recalibration wait time (before and after FRC command)
#define ITV_SEND (10 * 60)     // Data send interval in seconds
#define ITV_WAIT (8.5 * 60)    // Waiting phase interval in seconds
#define ITV_MEASURE (10)       // Measurement interval in seconds
#define ITV_MEASURE_WAIT (120) // Measurement interval in seconds (during wait phase)

const byte pinButton = D6;

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
ulong msSend = 0;                      // Last send time

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

void printTime()
{
    ulong ms = millis() - msSend;
    ulong totalSeconds = (ms + 500) / 1000; // Round milliseconds to nearest second
    ulong minutes = (totalSeconds / 60) % 60;
    ulong seconds = totalSeconds % 60;
    Serial.print("Uptime: ");
    if (minutes < 10)
        Serial.print('0');
    Serial.print(minutes);
    Serial.print(':');
    if (seconds < 10)
        Serial.print('0');
    Serial.println(seconds);
    Serial.printf("Measurement interval %u seconds.\n", airSensor.getMeasurementInterval());
}

enum Phase
{
    Wait,
    Measure,
};
Phase phase;

void setup()
{
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    pinMode(pinButton, INPUT_PULLUP);
    Serial.begin(115200);
    Serial.println();
    Wire.begin();
    while (!airSensor.begin())
    {
        Serial.println("Air sensor not detected. Please check wiring. Freezing...");
        ledOnRestart();
    }
    airSensor.setAutoSelfCalibration(false);
    Serial.printf("Auto calibration is %s.\n", airSensor.getAutoSelfCalibration() ? "ON" : "OFF");
    airSensor.setAltitudeCompensation(170); // Set altitude compensation to 170m
    Serial.printf("Altitude compensation set to %u meters.\n", airSensor.getAltitudeCompensation());
#ifdef USE_ESP_NOW
    Serial.println("Sending data via ESP-NOW.");
#endif

    airSensor.setMeasurementInterval(2);
    ledOn(true);
    while (!airSensor.dataAvailable())
    {
        Serial.print('.');
        delay(500);
    }
    Serial.println();
    ledOn(false);

    // When sketch starts, if btn pressed - FRC, else - normal operation.
    if (digitalRead(pinButton) == LOW) // button pressed -> FRC
    {
        ulong msStartFrc = millis(); // Start time for forced calibration
        Serial.println("Button held on startup, starting forced recalibration with 425 ppm reference.");
        airSensor.setMeasurementInterval(ITV_MEASURE);
        // LED is blinking before sending FRC command (10 minutes)
        auto isLedOn = false;
        while (millis() < msStartFrc + ITV_FRC * SECOND)
        {
            delay(2000);
            ledOn(isLedOn = !isLedOn);
        }
        msStartFrc = millis();                       // Reset start time for FRC
        airSensor.setForcedRecalibrationFactor(425); // Set forced recalibration factor to 425 ppm
        Serial.println("Forced recalibration with 425 ppm reference command sent to sensor.");
        // LED is ON after sending FRC command (10 minutes)
        ledOn(true);
        while (millis() < msStartFrc + ITV_FRC * SECOND)
            delay(1000);
        ledOn(false);
    }
    else
        Serial.println("Normal startup, reading and sending values to the hub.");

    phase = Wait;
    airSensor.setMeasurementInterval(ITV_MEASURE_WAIT);
    Serial.printf("Initial wait phase started for %u seconds.\n", airSensor.getMeasurementInterval());
    msSend = millis();
    printTime();

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
}

void loop()
{
    if (phase == Wait)
    {
        if (airSensor.dataAvailable())
        {
            Serial.println("Wait phase - data available.");
            // airSensor.getCO2();
            airSensor.readMeasurement(); // Read to clear data available flag
            printTime();
        }
        if (millis() > msSend + ITV_WAIT * SECOND)
        {
            phase = Measure;
            Serial.println(" * Measurement phase");
            airSensor.setMeasurementInterval(ITV_MEASURE);
            printTime();
        }
    }
    else if (phase == Measure)
    {
        if (airSensor.dataAvailable())
        {
            auto co2 = airSensor.getCO2();
            auto temp = airSensor.getTemperature();
            auto hum = airSensor.getHumidity();
            printValues(co2, temp, hum);
            prevCO2 = co2;
            prevTemp = temp;
            prevHum = hum;
            printTime();
        }
        // send data every ITV_SEND milliseconds, if CO2 is valid (not zero)
        if (millis() > msSend + (ITV_SEND - 0.06) * SECOND)
        {
            if (digitalRead(pinButton) == LOW) // button pressed -> skip send
                ledOn(true);                   // LED will be turned ON while button is pressed (waiting to send data)
            else
            {
                ledOn(false);
                Serial.println(" *** Sending data...");
                printValues(prevCO2, prevTemp, prevHum);
#ifdef USE_ESP_NOW
                if (prevCO2 != 0)
                {
                    Serial.println("Sending data via ESP-NOW");
                    airData.ECO2 = prevCO2;
                    airData.temperature = prevTemp;
                    airData.humidity = prevHum + 0.5f; // Round humidity
                    auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
                    printf("Send res: 0x%X\n", res);
                    if (res != 0)
                        ledOnDelay(10);
                }
                else
                    Serial.println("CO2 is zero, skipping ESP-NOW send.");
#endif
                phase = Wait;
                Serial.println(" * Wait phase");
                airSensor.setMeasurementInterval(ITV_MEASURE_WAIT);
                msSend = millis();
                printTime();
            }
        }
    }
    delay(20);
}
