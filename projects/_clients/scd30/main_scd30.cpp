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

#define TESTING false

// https://sensirion.com/media/documents/0FEA2450/61652EF9/Sensirion_CO2_Sensors_SCD30_Low_Power_Mode.pdf
#define ITV_FRC ((TESTING ? 1 : 10) * 60)     // Forced Recalibration wait time (before and after FRC command)
#define ITV_SEND ((TESTING ? 2 : 10) * 60)    // Data send interval in seconds
#define ITV_WAIT ((TESTING ? 1 : 8.5) * 60)   // Waiting phase interval in seconds
#define ITV_MEASURE (10)                      // Measurement interval in seconds
#define ITV_MEASURE_WAIT (TESTING ? 20 : 120) // Measurement interval in seconds (during wait phase)
#define SEC_REPEAT_SEND_DELAY 3               // Interval in seconds between send attempts
#define MAX_SEND_ATTEMPTS 3

const byte pinButton = D6;

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SECOND);
    ledOn(false);
}
void blinks(int times, int msInterval)
{
    for (int i = 0; i < times; i++)
    {
        ledOn(true);
        delay(msInterval);
        ledOn(false);
        delay(msInterval);
    }
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

#include <LittleFS.h>
void logEvent(const String &msg)
{
    Serial.println(msg);
    File f = LittleFS.open("/log.txt", "a");
    if (f)
    {
        auto minutes = millis() / 1000 / 60.0f;
        f.printf("[%.1f min]\t\t", minutes);
        f.println(msg);
        f.close();
    }
}
void printLog()
{
    File f = LittleFS.open("/log.txt", "r");
    while (f.available())
        Serial.write(f.read());
    f.close();
}
// TODO clear log - click: print log, double click: clear log

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

void printTime(bool printItvMeasurement = true)
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
    Serial.print(seconds);
    if (printItvMeasurement)
        Serial.printf("\tMeasurement interval: %u seconds.", airSensor.getMeasurementInterval());
    Serial.println();
}

enum Phase
{
    Wait,
    Measure,
};
Phase phase;

int cntSendRetries = 0;
bool isDataSent = false;

void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    if (sendStatus != 0)
        logEvent("ESP-NOW last packet send FAILED, status: " + String(sendStatus));
    else
        cntSendRetries++;
    if (sendStatus == 0 || cntSendRetries >= MAX_SEND_ATTEMPTS)
    {
        phase = Wait;
        Serial.println(" * Wait phase");
        airSensor.setMeasurementInterval(ITV_MEASURE_WAIT);
        msSend = millis();
        printTime(false);
        cntSendRetries = 0;
    }
    isDataSent = false;
}

void setup()
{
    LittleFS.begin();
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    pinMode(pinButton, INPUT_PULLUP);
    Serial.begin(115200);
    Serial.println();
    Wire.begin();
    while (!airSensor.begin())
    {
        logEvent("Air sensor not detected on startup.");
        ledOnRestart();
    }
    airSensor.setAutoSelfCalibration(false);
    Serial.printf("Auto calibration is %s.\n", airSensor.getAutoSelfCalibration() ? "ON" : "OFF");
    airSensor.setTemperatureOffset(3);
    Serial.printf("Temperature offset set to %.1f C.\n", airSensor.getTemperatureOffset());
    airSensor.setAltitudeCompensation(170); // Set altitude compensation to 170m
    Serial.printf("Altitude compensation set to %u meters.\n", airSensor.getAltitudeCompensation());

    // When sketch starts, if btn pressed - FRC, else - normal operation.
    auto isFrc = (digitalRead(pinButton) == LOW);
    if (isFrc)
    {
        Serial.println("Button held on startup, starting forced recalibration with 425 ppm reference.");
        blinks(5, 200);
    }
    else
        Serial.println("Normal startup mode.");

    Serial.print("Waiting for first measurement data...");
    airSensor.setMeasurementInterval(2);
    ledOn(true);
    while (!airSensor.dataAvailable())
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println();
    ledOn(false);

    if (isFrc)
    {
        ulong msStartFrc = millis(); // Start time for forced calibration
        // Serial.println("Button held on startup, starting forced recalibration with 425 ppm reference.");
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
        // Serial.println("ESP NOW INIT FAIL");
        logEvent("ESP-NOW init error");
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
    esp_now_register_send_cb(OnDataSent);
    auto res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    if (res != 0)
    {
        // printf("esp_now_add_peer res: 0x%X\n", res);
        logEvent("ESP-NOW add peer error: " + String(res));
        ledOnRestart();
    }
#endif
#endif
}

bool isBtnPressed = false, isBtnPressedPrev = false;

void loop()
{
    isBtnPressed = digitalRead(pinButton) == LOW;
    if (isBtnPressed && !isBtnPressedPrev)
    {
        Serial.println("Button pressed - displaying log:");
        printLog();
    }

    if (phase == Wait)
    {
        if (airSensor.dataAvailable())
        {
            Serial.println("Wait phase - data available.");
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
        if (millis() > msSend + ((ITV_SEND - 0.06) + cntSendRetries * SEC_REPEAT_SEND_DELAY) * SECOND && !isDataSent)
        {
            if (isBtnPressed) // button pressed -> skip send
                ledOn(true);  // LED will be turned ON while button is pressed (waiting to send data)
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
                    isDataSent = true;
                    if (res != 0)
                    {
                        // printf("Send res: 0x%X\n", res);
                        logEvent("ESP-NOW send error: " + String(res));
                        ledOnDelay(10);
                    }
                }
                else
                    Serial.println("CO2 is zero, skipping ESP-NOW send.");
#endif
                // phase = Wait;
                // Serial.println(" * Wait phase");
                // airSensor.setMeasurementInterval(ITV_MEASURE_WAIT);
                // msSend = millis();
                // printTime();
            }
        }
    }
    isBtnPressedPrev = isBtnPressed;
    delay(20);
}
