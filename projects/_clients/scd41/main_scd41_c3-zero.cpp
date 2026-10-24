//* SCD41 sensor node; sends air quality data (CO2, temperature, humidity) via ESP-NOW to the hub.
//* TODO Make code more robust - put some checks and retries

#include <Arduino.h>

// const uint8_t pinBtn = 7;
// const uint8_t pinLED = 8; // built-in LED on ESP32-C3 Pro Mini
// const uint8_t pinSCL = 9;
// const uint8_t pinSDA = 10;

const uint8_t pinBtn = 9;
const uint8_t pinLED = 10; // built-in RGB LED on ESP32-C3 Zero
const uint8_t pinSCL = 2;
const uint8_t pinSDA = 1;

#define SECOND (1000UL)
#define ITV_FRC_PRE (5 * 60) // Forced Recalibration wait time (before FRC command)
// #define ITV_FRC_ERROR (2 * 60) // Forced Recalibration - LED blinking time if error occured

#include "OneButton.h"       // lib_deps = mathertel/OneButton@^2.0.0
OneButton btn(pinBtn, true); // click: send data, 2click: print log, long click: clear log

#include <Adafruit_NeoPixel.h> // adafruit/Adafruit NeoPixel @ ^1.15.5
Adafruit_NeoPixel strip(1, pinLED, NEO_GRB + NEO_KHZ800);
const uint32_t ledColorGreen = Adafruit_NeoPixel::Color(0, 255, 0);
const uint32_t ledColorRed = Adafruit_NeoPixel::Color(255, 0, 0);
void ledStart()
{
    strip.begin();
    strip.setBrightness(10);
    strip.clear();
    strip.show();
}
void ledOn(bool on, ulong ledColor = ledColorRed)
{
    if (on)
        strip.setPixelColor(0, ledColor);
    else
        strip.clear();
    strip.show();
}

// void ledOn(bool on) { digitalWrite(pinLED, !on); }
// void ledOnDelay(int secs)
// {
//     ledOn(true);
//     delay(secs * SECOND);
//     ledOn(false);
// }
void ledOnBlink(int times, int msInterval, uint32_t ledColor = ledColorRed)
{
    for (int i = 0; i < times; i++)
    {
        ledOn(true, ledColor);
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
// void ledOnRestart()
// {
//     ledOnDelay(10);
//     ESP.restart();
// }

#include <Wire.h>
#include <SensirionI2cScd4x.h>
SensirionI2cScd4x sensor;

static char errorMessage[64]; // buffer for error message from SCD41 functions
static int16_t error;         // result (error code) of SCD41 functions, 0 = no error

enum Phase
{
    WaitPhase,    // Waiting
    PreparePhase, // Get time[, measure] - rename to ReadyPhase?
    PhaseCount
};
Phase phase;
void nextPhase()
{
    // phase = (phase == WaitPhase) ? PreparePhase : WaitPhase;
    phase = static_cast<Phase>((static_cast<int>(phase) + 1) % PhaseCount);
    // Serial.println(String("Next phase: ") + (phase == WaitPhase ? "WaitPhase" : "PreparePhase"));
}

#include "AirData.h"
AirData airData;

#include "MacAddresses.h"
#include <esp_now.h>
#include <WiFi.h>
uint8_t *mac = macSoftEsp32BanovoBrdoHub;
// uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing
esp_now_peer_info_t peerInfo;

#include "TimeSlotSend.h"
#define ITV_TIME_SLOT_MIN (10)  // Time slot in minutes (send data every ITV_TIME_SLOT_MIN minutes at slotSec seconds after the minute mark)
#define ITV_TIME_SLOT_SEC (15) // Seconds after the minute mark to send data (e.g. 5 means send at 11:00:05, 11:10:05...)
TimeSlotSend tss(ITV_TIME_SLOT_MIN, ITV_TIME_SLOT_SEC, 15, 150, 60);

int cntTimeReqRetries = 0; // Count of fast time request retries
ulong msTimeReqSent = 0;
void sendTimeRequest()
{
    if (msTimeReqSent == 0 || millis() > msTimeReqSent + 5 * SECOND)
    {
        if (millis() - msTimeReqSent < 10 * SECOND)
        {
            cntTimeReqRetries++;
            if (cntTimeReqRetries > 3)
            {
                Serial.println("Time request retries exceeded");
                ledOnForever();
            }
        }
        else
            cntTimeReqRetries = 0;
        msTimeReqSent = millis();
        Serial.println(String("sendTimeRequest @ ") + tss.getCurrentTime(millis()));
        // tss.timeReqIsSent(millis());
        auto res = esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
        if (res != ESP_OK)
            Serial.println("ESP-NOW time request send error: " + String(res));
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    tss.onTimeStringRecv(incomingData, len, millis(), true);
}

// If there is an error returned from some SCD41 function - print error message to Serial
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
    ledStart();

    Wire.begin(pinSDA, pinSCL);
    sensor.begin(Wire, SCD41_I2C_ADDR_62);
    delay(100);

    error = sensor.wakeUp();
    handleSensorErrorIN();
    error = sensor.stopPeriodicMeasurement();
    handleSensorErrorIN();

    ulong msPressBtnForFRC = millis(); // User should press a button if they want to perform forced recalibration (FRC) on startup
    ledOn(true, ledColorGreen);
    bool startFRC = false;
    while (millis() < msPressBtnForFRC + 5 * SECOND)
    {
        if (digitalRead(btn.pin()) == LOW)
            startFRC = true;
        delay(20);
    }
    ledOn(false);
    if (startFRC)
    {
        Serial.println("Forced recalibration (FRC)...");
        ledOnBlink(5, 200, ledColorGreen);
        auto msStartFrc = millis(); // Start time for forced calibration
        error = sensor.startLowPowerPeriodicMeasurement();
        handleSensorErrorIN();
        ledOnBlink(ITV_FRC_PRE / 4, 2 * SECOND, ledColorGreen); // 4 = 2 intervals (ON and OFF) * 2 seconds
        error = sensor.stopPeriodicMeasurement();
        handleSensorErrorIN();
        delay(SECOND);
        uint16_t frcCorrection;
        sensor.performForcedRecalibration(425, frcCorrection);
        delay(SECOND);
        if (error != NoError || frcCorrection == 0xFFFF)
            ledOnForever();
    }
    else
        Serial.println("Normal startup mode.");

    error = sensor.startLowPowerPeriodicMeasurement();
    handleSensorErrorIN();
    Serial.println("SCD41 low-power mode started");
    // lastReport = millis();

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
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_13dBm); // adjust power for wifi antenna, default is max power
    while (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP NOW INIT FAIL");
        ledOnForever();
    }
    // auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    auto res = esp_now_register_recv_cb(OnDataRecv);
    if (res != ESP_OK)
    {
        Serial.printf("Register receiver code: %X\n", res);
        // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        // esp_now_register_send_cb(OnDataSent);
        ledOnForever();
    }

    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0; // use current channel
    peerInfo.encrypt = false;
    res = esp_now_add_peer(&peerInfo);
    if (res != ESP_OK)
    {
        printf("esp_now_add_peer res: 0x%X\n", res);
        ledOnForever();
    }
    sendTimeRequest();
    phase = PreparePhase;
    Serial.println("Setup complete");
}

// ulong msLastTssCheck = 0;
ulong msNextTimeReq = 0;

void loop()
{
    delay(20);
    btn.tick();
    auto ms = millis();
    // if (ms > msLastTssCheck + 1000) //* testing
    // {
    //     msLastTssCheck = ms;
    //     printf("%.1f\t%.1f\n", ms / 1000.0, tss.getMsTimeToSendData() / 1000.0);
    // }
    if (phase == WaitPhase)
    {
        if (ms > msNextTimeReq)
            nextPhase();
        else
            return;
    }

    // if (tss.getMsTimeReqSent() == 0)
    if (msTimeReqSent == 0)
        sendTimeRequest();
    else if (millis() > msTimeReqSent + 5 * SECOND)
    {
        if (tss.getStrTime()[0] == '\0')
            sendTimeRequest();
    }

    if (!tss.isTimeToSendData(ms) || ms < msNextTimeReq + 5 * SECOND)
        return;

    bool isDataReady = false;
    ulong startTime = ms;
    const ulong TIMEOUT_MS = 6000;
    // Poll until data is ready or timeout expires
    while (!isDataReady && (millis() < startTime + TIMEOUT_MS))
    {
        delay(200);                             // yield to avoid hammering the I2C bus
        sensor.getDataReadyStatus(isDataReady); //? Check !error here? If error, break and report?
    }
    if (isDataReady)
    {
        uint16_t co2;
        float temp = 0.0f, humidity = 0.0f;
        error = sensor.readMeasurement(co2, temp, humidity);
        if (!error)
        {
            Serial.printf("CO2: %d ppm | T: %.2f °C | RH: %.2f %%\n", co2, temp, humidity);
            airData.ECO2 = co2;
            airData.temperature = temp;
            airData.humidity = humidity + 0.5f; // Round humidity
            printf("Sending data via ESP-NOW, millis: %lu\t%s\n", millis(), tss.getCurrentTime(millis()));
            auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
            if (res != 0)
                Serial.println("ESP-NOW send error: " + String(res));
            if (ITV_TIME_SLOT_MIN - 1 > 0)
                msNextTimeReq = millis() + (ITV_TIME_SLOT_MIN - 1) * 60 * SECOND;
            else
                ledOnForever();
            // tss.resetMsTimeReqSent();
            msTimeReqSent = 0;
            nextPhase();
        }
    }
}
