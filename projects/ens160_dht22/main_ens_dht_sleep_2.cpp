//* ESP8266 initializes module with ENS160 and DHT22 sensors, reads their values and sends them to the hub via ESP-NOW.

#include <DHT.h>      // lib_deps = adafruit/DHT sensor library@^1.4.6
#define DHTPIN D5     // DHT sensor pin on ESP8266 D1 Mini Lite
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#include <DFRobot_ENS160.h>
#define I2C_COMMUNICATION
DFRobot_ENS160_I2C ens(&Wire, /*I2CAddr*/ 0x53);

#include "AirData.h"
AirData airData;
bool isInWarmUpPhase() { return airData.status == 1; } // Check if ENS160 is in warm-up phase

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

#include "MacAddresses.h"
#include <espnow.h>
#include <ESP8266WiFi.h>
uint8_t *mac = macSoftEsp32DevIpex;

#include "TimeSlotSend.h"
TimeSlotSend tss(10, 10, -20, 15, 60);

void sendTimeRequest()
{
    esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
    tss.timeReqIsSent(millis());
}

void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    if (sendStatus != 0)
        Serial.println("Last Packet Send Status: FAIL!!!");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    tss.onTimeStringRecv(incomingData, len, millis(), true);
}

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
        ledOnDelay(10);
    }
    ens.setPWRMode(ENS160_STANDARD_MODE);
    // ens.setTempAndHum(25.0, 50.0); // Set default temperature and humidity
    // // TODO set temp and hum to last known values from NVS

    WiFi.mode(WIFI_STA);
    while (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        ledOnDelay(10);
    }
    auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    if (res != 0)
        Serial.printf("Register receiver code: %X\n", res);
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    if (res != 0)
        printf("esp_now_add_peer res: 0x%X\n", res);
    if (res != 0)
        ledOnDelay(10);
    else
        sendTimeRequest();
}

void loop()
{
    if (tss.isTimeToSendData(millis()))
    {
        do
        {
            // printf("Reading sensors millis: %lu\n", millis());
            cntRetries = 0;
            shouldRetry = false;
            // DHT22
            float hum = dht.readHumidity();
            float temp = dht.readTemperature();
            if (isnan(hum) || isnan(temp))
            {
                Serial.println("Failed to read from DHT sensor!");
                // ens.setTempAndHum(25.0, 50.0); // Set default temperature and humidity
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

            // ENS160
            ens.setTempAndHum(temp, hum); // Set temperature and humidity for ENS160
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
            if (airData.AQI == 0 || isInWarmUpPhase())
                shouldRetry = true;
            if (shouldRetry)
            {
                cntRetries++;
                Serial.printf("Retrying... (%d/%d)\n", cntRetries, maxRetries);
                ledOnDelay(10);
                if (isInWarmUpPhase())
                    ledOnDelay(50); // If in warm-up phase, wait longer
            }
        } while (shouldRetry && (cntRetries < maxRetries || isInWarmUpPhase()));

        // printf("Sending data millis: %lu\n", millis());
        if (cntRetries < maxRetries)
        {
            Serial.println("Sending data via ESP-NOW");
            auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
            if (res != 0)
            {
                printf("Send res: 0x%X\n", res);
                ledOnDelay(5);
            }
        }
        delay(100); // wait for send callback
        Serial.println("GO TO SLEEP");
        ESP.deepSleep(tss.getDeepSleepTime());
    }
    // repeat sendTimeRequest() if the answer (time) is not received for more than 1 sec
    if (tss.isTimeRespMissing(millis()))
        sendTimeRequest();
    if (tss.getIsWakeUpTimeWrong())
    {
        Serial.println("Wake-up time is off!");
        for (size_t i = 0; i < 20; i++)
        {
            ledOn(i % 2);
            delay(100);
        } // LED stays ON after this loop - indication of wake-up time off
        tss.resetWakeUpTimeWrong();
    }
    delay(250);
}
