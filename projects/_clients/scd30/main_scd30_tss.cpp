//*

#include "Arduino.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" // sparkfun/SparkFun SCD30 Arduino Library@^1.0.20
SCD30 airSensor;

#include "AirData.h"
AirData airData;
#define SECOND (1000UL)
#define MINUTE (60 * SECOND)

#define ITV_FRC (10 * 60)      // Forced Recalibration wait time (before and after FRC command)
#define ITV_TIME_SLOT_MIN (10) // Time slot in minutes (send data every ITV_TIME_SLOT_MIN minutes at slotSec seconds after the minute mark)
#define ITV_TIME_SLOT_SEC (5)  // Seconds after the minute mark to send data (e.g. 5 means send at 11:00:05, 11:10:05...)
#define ITV_WAIT (8.5 * 60)    // Waiting phase interval in seconds
#define ITV_MEASURE_WAIT (120) // Measurement interval in seconds (during wait phase)
#define ITV_MEASURE (10)       // Measurement interval in seconds

const byte maxRetries = 3; // Max retries for getting data from sensors
byte cntRetries = 0;       // Counter for retries
bool shouldRetry = false;  // Flag to indicate if we should retry reading sensors

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SECOND);
    ledOn(false);
}
void ledOnBlink(int times, int msInterval)
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
        delay(SECOND);
}
void ledOnRestart()
{
    ledOnDelay(10);
    ESP.restart();
}

#include "MacAddresses.h"
#include <espnow.h>
#include <ESP8266WiFi.h>
uint8_t *mac = macSoftEsp32DevIpex;
uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing

#include "TimeSlotSend.h"
TimeSlotSend tss(ITV_TIME_SLOT_MIN, ITV_TIME_SLOT_SEC, 0, 0, 0);
#include "ClientLogger.h"
ClientLogger logger;
#include "OneButton.h"   // lib_deps = mathertel/OneButton@^2.0.0
OneButton btn(D6, true); // click: send data, 2click: print log, long click: clear log
bool sendDataNow = false;

// int prevCO2 = 0;                       // Previous CO2 value
// float prevTemp = 0.0f, prevHum = 0.0f; // Previous temperature and humidity values
ulong msSend = 0;                      // Last send time
ulong msDataSent = 0;
bool isInWaitPhase = false;

void printValues(uint16_t co2, float temp, float hum)
{
    Serial.print("Displaying CO2: ");
    Serial.print(co2);
    Serial.print(" ppm, Temp: ");
    Serial.print(temp, 1);
    Serial.print(" C, Humidity: ");
    Serial.print(hum, 1);
    Serial.print(" %");
}

// D
// void printTime(bool printItvMeasurement = true)
// {
//     ulong ms = millis() - msSend;
//     ulong totalSeconds = (ms + 500) / 1000; // Round milliseconds to nearest second
//     ulong minutes = (totalSeconds / 60) % 60;
//     ulong seconds = totalSeconds % 60;
//     Serial.print("Uptime: ");
//     if (minutes < 10)
//         Serial.print('0');
//     Serial.print(minutes);
//     Serial.print(':');
//     if (seconds < 10)
//         Serial.print('0');
//     Serial.print(seconds);
//     if (printItvMeasurement)
//         Serial.printf("\tMeasurement interval: %u seconds.", airSensor.getMeasurementInterval());
//     Serial.println();
// }

void goToWaitPhase(bool b)
{
    if (b == isInWaitPhase)
        return;
    if (tss.getSlotMin() < 10)
        return; // if slotMin is less than 10 minutes, we stay in measure phase and ignore wait phase
    isInWaitPhase = b;
    airSensor.setMeasurementInterval(b ? ITV_MEASURE_WAIT : ITV_MEASURE);
    Serial.println((b ? "Entering wait phase" : "Entering measure phase") + String(" @ ") + tss.getCurrentTime(millis()));
}

void sendTimeRequest()
{
    Serial.println(String("sendTimeRequest @ ") + tss.getCurrentTime(millis()));
    // printTime(false);
    auto res = esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
    if (res != 0)
        logger.add("ESP-NOW time request send error: " + String(res));
    tss.timeReqIsSent(millis());
}

void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    if (sendStatus != 0)
        logger.add("ESP-NOW last packet send FAILED, status: " + String(sendStatus));
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    tss.onTimeStringRecv(incomingData, len, millis(), true);
}

// forced re-calibration for scd30 sensor
void frc()
{
    auto msStartFrc = millis(); // Start time for forced calibration
    // Serial.println("Button held on startup, starting forced recalibration with 425 ppm reference.");
    airSensor.setMeasurementInterval(10);
    auto isLedOn = false;
    // LED is blinking before sending FRC command (10 minutes)
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

void setup()
{
    Serial.begin(115200);
    Serial.println();
    pinMode(pinLed, OUTPUT);
    ledOn(false);

    Wire.begin();
    while (!airSensor.begin())
    {
        logger.add("Air sensor not detected on startup.");
        ledOnRestart();
    }
    airSensor.setAutoSelfCalibration(false);
    Serial.printf("Auto calibration is %s.\n", airSensor.getAutoSelfCalibration() ? "ON" : "OFF");
    airSensor.setTemperatureOffset(3);
    Serial.printf("Temperature offset set to %.1f C.\n", airSensor.getTemperatureOffset());
    airSensor.setAltitudeCompensation(170); // Set altitude compensation to 170m
    Serial.printf("Altitude compensation set to %u meters.\n", airSensor.getAltitudeCompensation());
    airSensor.setMeasurementInterval(ITV_MEASURE);
    Serial.printf("Measurement interval set to %u seconds.\n", airSensor.getMeasurementInterval());

    // When sketch starts, if btn pressed - FRC, else - normal operation.
    auto isFrc = digitalRead(btn.pin()) == LOW;
    if (isFrc)
    {
        Serial.println("Button held on startup, starting forced recalibration with 425 ppm reference.");
        ledOnBlink(5, 200);
    }
    else
        Serial.println("Normal startup mode.");
    Serial.print("Waiting for first measurement data...");
    while (!airSensor.dataAvailable())
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println();
    if (isFrc)
        frc();

    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        logger.add("ESP-NOW init error", tss.getCurrentTime(millis()));
        ledOnDelay(10);
    }
    auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    if (res != 0)
        logger.add("ESP-NOW register recv cb error: " + String(res), tss.getCurrentTime(millis()));
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    if (res != 0)
    {
        logger.add("ESP-NOW add peer error: " + String(res), tss.getCurrentTime(millis()));
        ledOnDelay(10);
    }
    else
        sendTimeRequest();

    btn.attachClick([]()
                    { sendDataNow = true; });

    btn.attachDoubleClick([]()
                          { logger.print(); });

    btn.attachLongPressStart([]()
                             { logger.clear(); });
}

void loop()
{
    if (airSensor.dataAvailable())
    {
        airSensor.readMeasurement();
        // auto co2 = airSensor.getCO2();
        // auto temp = airSensor.getTemperature();
        // auto hum = airSensor.getHumidity();
        // printValues(co2, temp, hum);
        // prevCO2 = co2;
        // prevTemp = temp;
        // prevHum = hum;
        // printTime();
        airData.ECO2 = airSensor.getCO2();
        airData.temperature = airSensor.getTemperature();   
        airData.humidity = airSensor.getHumidity() + 0.5f; // Round humidity
        printValues(airData.ECO2, airData.temperature, airData.humidity);
        Serial.println(String(" @ ") + tss.getCurrentTime(millis()));
    }
    // if (tss.isTimeToSendData(millis()))
    if (tss.isTimeToSendData(millis()) || sendDataNow)
    {
        // airData.ECO2 = prevCO2;
        // airData.temperature = prevTemp;
        // airData.humidity = prevHum + 0.5f; // Round humidity
        if (airData.ECO2 == 0)
            logger.add("DCO2 value is 0, likely sensor read error. Data won't be sent.", tss.getCurrentTime(millis()));
        else
        {
            printf("Sending data via ESP-NOW, millis: %lu\t%s\n", millis(), tss.getCurrentTime(millis()));
            auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
            if (res != 0)
                logger.add("ESP-NOW send error: " + String(res), tss.getCurrentTime(millis()));
            msDataSent = millis();
            goToWaitPhase(true);
        }
        delay(2000);
        if (sendDataNow)
            sendDataNow = false;
        sendTimeRequest();
    }
    // if (tss.isTimeRespMissing(millis()))
    //     sendTimeRequest();
    if (millis() - msDataSent > ITV_WAIT * SECOND)
        goToWaitPhase(false);

    delay(20);
    btn.tick();
}

/*
void loop()
{
    if (tss.isTimeToSendData(millis()) || sendDataNow)
    {
        cntRetries = 0;
        do
        {
            printf("Reading sensors millis: %lu\n", millis());
            if (airSensor.dataAvailable())
            {
                airSensor.readMeasurement();
                auto co2 = airSensor.getCO2();
                auto temp = airSensor.getTemperature();
                auto hum = airSensor.getHumidity();
                printValues(co2, temp, hum);
                prevCO2 = co2;
                prevTemp = temp;
                prevHum = hum;
                airData.ECO2 = co2;
                airData.temperature = temp;
                airData.humidity = hum + 0.5f; // Round humidity
                printTime();
            }
            // if (airData.AQI == 0 || isInWarmUpPhase())
            //     shouldRetry = true;
            // if (shouldRetry)
            // {
            //     cntRetries++;
            //     // Serial.printf("Retrying... (%d/%d)\n", cntRetries, maxRetries);
            //     logger.add("Retrying sensor read... (" + String(cntRetries) + "/" + String(maxRetries) + ")", tss.getStrTime());
            //     delay(1000);
            //     ledOnDelay(10);
            // }
        } while (shouldRetry && (cntRetries < maxRetries));

        printf("Sending data millis: %lu\n", millis());
        if (cntRetries < maxRetries)
        {
            if (airData.ECO2 == 0)
                logger.add("DCO2 value is 0, likely sensor read error. Data won't be sent.", tss.getStrTime());
            else
            {
                Serial.println("Sending data via ESP-NOW");
                auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
                if (res != 0)
                    logger.add("ESP-NOW send error: " + String(res), tss.getStrTime());
            }
            ledOnDelay(5);
            sendTimeRequest();
        }
        // if data is sent on click - do not go to sleep and wait for time slot to send data
        if (sendDataNow)
            sendDataNow = false;
        // else
        // {
        //     delay(100); // wait for send callback
        //     Serial.println("GO TO SLEEP");
        //     ESP.deepSleep(tss.getDeepSleepTime());
        // }
    }
    // repeat sendTimeRequest() if the answer (time) is not received for more than 1 sec
    // if (tss.isTimeRespMissing(millis()))
    //     sendTimeRequest();
    // if (tss.getIsWakeUpTimeWrong())
    // {
    //     logger.add("Wake-up time is off! Diff: " + String(tss.getItvWrongTimeDiff()) + " seconds", tss.getStrTime());
    //     // if ESP woke up just a bit too late - send data anyway

    //     // e.g. more than 9 minutes = less than 1 minute late
    //     Serial.println(tss.getSlotMin() * 60 - 30);
    //     if (tss.getItvWrongTimeDiff() > tss.getSlotMin() * 60 - 30)
    //     {
    //         sendDataNow = true;
    //     }
    //     else
    //     {
    //         for (size_t i = 0; i <= 20; i++)
    //         {
    //             ledOn(i % 2);
    //             delay(100);
    //         }
    //     }
    //     tss.resetWakeUpTimeWrong();
    // }
    delay(20);
    btn.tick();
}
*/