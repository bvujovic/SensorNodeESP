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

#define TIME_SLOT (10)   // (seconds) Data sending time slot after the n-minute mark (e.g. 5): 11:00:05, 11:10:05...
#define ITV_SEND (10)    // (minutes) Send data every ITV_SEND minutes
#define SLEEP_TIME (9.8) // (minutes) Time to sleep between data sends
//* SLEEP_TIME should be (a little) less than ITV_SEND to wake up before next mark
char cmdTime[] = "time";
ulong msSendData = 0; // Time to send data from sensors

int secondsToNextMark(const char *timeStr, int markMin)
{
    int h, m, s;
    // Parse "HH:MM:SS"
    if (sscanf(timeStr, "%d:%d:%d", &h, &m, &s) != 3)
        return -1; // invalid input

    // Total seconds from start of hour
    int secNow = m * 60 + s;

    // Next 10-minute mark (0, 10, 20, 30, 40, 50)
    int nextMarkMin = ((m / markMin) + 1) * markMin;
    int nextMarkSec = nextMarkMin * 60;

    auto res = nextMarkSec - secNow;
    return res != markMin * 60 ? res : 0;
}

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
uint8_t *mac = macSoftEsp32DevIpex;
#endif
// void OnDataSent(uint8_t *mac, uint8_t res)
// {
//     Serial.printf("Send status: 0x%X\n", res);
//     if (res != 0)
//         ledOn_10sec();
// }
#endif

ulong msLastSendReqTime = 0; // Last time we sent time request

void sendTimeRequest()
{
    int res;
    byte timeRetries = 0;
    do
    {
        msLastSendReqTime = millis();
        printf("sendTimeRequest time: %lu\n", msLastSendReqTime);
        res = esp_now_send(mac, (uint8_t *)&cmdTime, strlen(cmdTime));
        printf("Initial esp_now_send res: 0x%X\n", res);
        if (res != 0)
            ledOnDelay(10);
    } while (res != 0 && ++timeRetries < maxRetries);
    if (timeRetries == maxRetries)
        msSendData = 1; // Force sending data without waiting for time response
}

char buf[80];

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    printf("OnDataRecv time: %lu\n", millis());
    int bufsize = sizeof(buf);
    auto n = len < bufsize ? len : bufsize - 1;
    memcpy(buf, incomingData, n);
    buf[n] = 0;
    printf("Received data: %s, len: %d\n", buf, len);
    // if it's response to time command - e.g. 16:25:01
    if (len == 8 && incomingData[2] == ':' && incomingData[5] == ':')
    {
        char s[10];
        memcpy(s, incomingData, len);
        s[len] = 0;
        // Serial.println(s);

        auto secs = secondsToNextMark(s, ITV_SEND);
        printf("Seconds to next %d-minute mark: %d\n", ITV_SEND, secs);
        if (secs < 0)
        {
            Serial.println("Error!!!");
            sendTimeRequest();
        }
        else
            // TODO should factor in time spent retrying reading sensors
            msSendData = millis() + (secs + TIME_SLOT) * 1000UL;
    }
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
    auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.printf("Register receiver code: %X\n", res);
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    // esp_now_register_send_cb(OnDataSent);
    res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    printf("esp_now_add_peer res: 0x%X\n", res);
    if (res != 0)
        ledOnDelay(10);
    else
    {
        sendTimeRequest();
    }
    // cntRetries++;
#endif
#endif
}

void loop()
{
    if (msSendData != 0 && millis() > msSendData)
    {
        do
        {
            shouldRetry = false;
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
            auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
            printf("Send res: 0x%X\n", res);
            if (res != 0)
                ledOnDelay(10);
        }
        delay(100); // wait for send callback
        ESP.deepSleep(1000000UL * 60 * SLEEP_TIME);
#else
        ESP.deepSleep(10 * SEC * 1000);
#endif
    }
    else
        delay(250);

    if (millis() - msLastSendReqTime > SEC && msSendData == 0)
    {
        Serial.println("No time response received, resending time request");
        sendTimeRequest();
    }
}
