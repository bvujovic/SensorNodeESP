
#include <Arduino.h>

const uint8_t pinBtn = 7;
const uint8_t pinLED = 8; // built-in LED on ESP32-C3 Pro Mini
const uint8_t pinSCL = 9;
const uint8_t pinSDA = 10;

#include <Wire.h>
#include <SensirionI2cScd4x.h>
SensirionI2cScd4x sensor;

static char errorMessage[64];
static int16_t error;

#define SECOND (1000UL)
#define ITV_FRC_PRE (5 * 60) // Forced Recalibration wait time (before FRC command)
// #define ITV_FRC_ERROR (2 * 60) // Forced Recalibration - LED blinking time if error occured

#include "AirData.h"
AirData airData;

// #include "MacAddresses.h"
// #include <esp_now.h>
// #include <WiFi.h>
// uint8_t *mac = macSoftEsp32BanovoBrdoHub;
// uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing
// esp_now_peer_info_t peerInfo;

// #include "TimeSlotSend.h"
// TimeSlotSend tss(2, 15, 15, 0, 60);

// void sendTimeRequest()
// {
//     Serial.println(String("sendTimeRequest @ ") + tss.getCurrentTime(millis()));
//     // printTime(false);
//     auto res = esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
//     if (res != ESP_OK)
//         Serial.println("ESP-NOW time request send error: " + String(res));
//     tss.timeReqIsSent(millis());
// }

// void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
// {
//     tss.onTimeStringRecv(incomingData, len, millis(), true);
// }

constexpr uint32_t REPORT_INTERVAL_MS = 2UL * 60UL * 1000UL;
uint32_t lastReport = 0;

// uint16_t co2 = 0;
// float temperature = 0;
// float humidity = 0;

#include "OneButton.h"       // lib_deps = mathertel/OneButton@^2.0.0
OneButton btn(pinBtn, true); // click: send data, 2click: print log, long click: clear log
// bool sendDataNow = false;

void ledOn(bool on) { digitalWrite(pinLED, !on); }
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

// void frcError() { ledOnBlink(ITV_FRC_ERROR, SECOND / 2); }

void handleSensorErrorIN()
{
    if (error != NoError)
    {
        errorToString(error, errorMessage, sizeof(errorMessage));
        Serial.printf("SCD41 error: %d, %s\n", error, errorMessage);
        ledOnForever();
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(pinLED, OUTPUT);
    ledOnDelay(2);
    Serial.println(" *** SCD41 node sensor");

    Wire.begin(pinSDA, pinSCL);
    sensor.begin(Wire, SCD41_I2C_ADDR_62);
    delay(100);

    // Ensure sensor is idle before selecting low-power mode.
    error = sensor.wakeUp();
    handleSensorErrorIN();
    error = sensor.stopPeriodicMeasurement();
    handleSensorErrorIN();
    delay(SECOND);

    // Check if button is held down on startup to trigger forced recalibration (FRC)
    if (digitalRead(btn.pin()) == LOW)
    {
        Serial.println("Forced recalibration (FRC)...");
        ledOnBlink(5, 200);
        auto msStartFrc = millis(); // Start time for forced calibration
        error = sensor.startLowPowerPeriodicMeasurement();
        handleSensorErrorIN();
        ledOnBlink(ITV_FRC_PRE / 4, 2 * SECOND); // 4 = 2 intervals (ON and OFF) * 2 seconds
        error = sensor.stopPeriodicMeasurement();
        handleSensorErrorIN();
        delay(SECOND);
        uint16_t frcCorrection;
        sensor.performForcedRecalibration(425, frcCorrection);
        delay(SECOND);
        if (error != NoError || frcCorrection == 0xFFFF)
            // ledOnBlink(ITV_FRC_ERROR, SECOND / 2);
            ledOnForever();
    }
    else
        Serial.println("Normal startup mode.");

    error = sensor.startLowPowerPeriodicMeasurement();
    handleSensorErrorIN();
    Serial.println("SCD41 low-power mode started");
    lastReport = millis();

    btn.attachDoubleClick(
        []()
        {
            error = sensor.stopPeriodicMeasurement();
            handleSensorErrorIN();

            float tempOffset = 0.0f;
            // sensor.setTemperatureOffset(tempOffset);
            auto error = sensor.getTemperatureOffset(tempOffset);
            if (!error)
                printf("Temperature offset set to %.2f °C\n", tempOffset);
            else
                printf("Failed to get temperature offset, error code: %d\n", error);

            uint16_t alt = 170;
            // sensor.setSensorAltitude(alt);
            error = sensor.getSensorAltitude(alt);
            if (!error)
                printf("Altitude: %d m\n", alt);
            else
                printf("Failed to get altitude, error code: %d\n", error);

            // print ASC data
            sensor.setAutomaticSelfCalibrationEnabled(false);
            uint16_t ascEnabled;
            error = sensor.getAutomaticSelfCalibrationEnabled(ascEnabled);
            if (!error)
                printf("ASC enabled: %d\n", ascEnabled);
            // else
            //   printf("Failed to get ASC status, error code: 0x%04X\n", error);

            // // sensor.setAutomaticSelfCalibrationInitialPeriod(4); // default is 44
            // uint16_t ascInitialPeriod;
            // error = sensor.getAutomaticSelfCalibrationInitialPeriod(ascInitialPeriod);
            // if (!error)
            //     printf("ASC initial period: %d\n", ascInitialPeriod);

            // // sensor.setAutomaticSelfCalibrationStandardPeriod(168); // default is 156
            // uint16_t ascStandardPeriod;
            // error = sensor.getAutomaticSelfCalibrationStandardPeriod(ascStandardPeriod);
            // if (!error)
            //     printf("ASC standard period: %d\n", ascStandardPeriod);

            // // sensor.setAutomaticSelfCalibrationTarget(430);
            // uint16_t ascTarget;
            // error = sensor.getAutomaticSelfCalibrationTarget(ascTarget);
            // if (!error)
            //     printf("ASC target: %d\n", ascTarget);

            //* Persist settings to EEPROM (optional, only needed if you want to save the current configuration)
            // sensor.persistSettings();
            // Serial.println("Settings persisted to EEPROM.");

            error = sensor.startLowPowerPeriodicMeasurement();
            handleSensorErrorIN();
        });

    // tss.setItvTimeRespWait(5000); // 5 sec to wait for time response
    // WiFi.mode(WIFI_STA);
    // WiFi.setTxPower(WIFI_POWER_13dBm); // adjust power for wifi antenna, default is max power
    // while (esp_now_init() != ESP_OK)
    // {
    //     Serial.println("ESP NOW INIT FAIL");
    //     ledOnForever();
    // }
    // // auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    // auto res = esp_now_register_recv_cb(OnDataRecv);
    // if (res != ESP_OK)
    // {
    //     Serial.printf("Register receiver code: %X\n", res);
    //     // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    //     // esp_now_register_send_cb(OnDataSent);
    //     ledOnForever();
    // }

    // memcpy(peerInfo.peer_addr, mac, 6);
    // peerInfo.channel = 0; // use current channel
    // peerInfo.encrypt = false;
    // res = esp_now_add_peer(&peerInfo);
    // if (res != ESP_OK)
    // {
    //     printf("esp_now_add_peer res: 0x%X\n", res);
    //     ledOnForever();
    // }
    // else
    //     sendTimeRequest();

    // Serial.println("Setup complete, waiting for time response...");
}

void loop()
{
    btn.tick();
    if (millis() - lastReport < REPORT_INTERVAL_MS)
    {
        delay(20);
        return;
    }
    // bool ready = false;
    // error = sensor.getDataReadyStatus(ready);
    // if (error != NoError)
    // {
    //     Serial.println("Failed to check data-ready status");
    //     lastReport = millis();
    //     return;
    // }
    // if (!ready)
    // {
    //     delay(500); // Try again shortly; don't restart the sensor.
    //     return;
    // }
    // error = sensor.readMeasurement(co2, temperature, humidity);
    // if (error == NoError)
    // {
    //     Serial.printf("CO2: %u ppm, ", co2);
    //     Serial.printf("Temperature: %.2f C, ", temperature);
    //     Serial.printf("Humidity: %.2f %%RH\n", humidity);
    // }
    // else
    // {
    //     Serial.println("Failed to read measurement");
    // }
    bool isDataReady = false;
    ulong startTime = millis();
    const ulong TIMEOUT_MS = 6000;
    // Poll until data is ready or timeout expires
    while (!isDataReady && (millis() < startTime + TIMEOUT_MS))
    {
        delay(200); // yield to avoid hammering the I2C bus
        sensor.getDataReadyStatus(isDataReady);
    }
    if (isDataReady)
    {
        uint16_t co2;
        float temp = 0.0f, humidity = 0.0f;
        error = sensor.readMeasurement(co2, temp, humidity);
        if (!error)
            Serial.printf("CO2: %d ppm | T: %.2f °C | RH: %.2f %%\n", co2, temp, humidity);
    }
    lastReport = millis();
}
