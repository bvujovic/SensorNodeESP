/*
 * Example of using Retryer class to manage retries and response timeouts for ESP-NOW communication.
 * The code attempts to send a "time" request to a peer device, retrying if necessary, and handles responses.
 * After a certain number of attempts, it enters deep sleep to save power.
 */

#include "Arduino.h"
#define SECOND (1000UL)
#define MINUTE (60 * SECOND)

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnDelay(int secs)
{
    ledOn(true);
    delay(secs * SECOND);
    ledOn(false);
}
void ledOnRestart()
{
    ledOnDelay(10);
    ESP.restart();
}

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
uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing

#include "Retryer.h"
Retryer retryer(2, 500, 1500); // 2 retries, 500ms between, 1.5s response timeout

void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    Serial.println("OnDataSent status: " + String(sendStatus));
    retryer.onSendResult(sendStatus == 0);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    ulong ms;
    memcpy(&ms, incomingData, sizeof(ms));
    Serial.println("OnDataRecv: " + String(ms) + " ms");
    // Serial.println("OnDataRecv: " + String((const char *)incomingData));
    retryer.onResponseReceived(); // optionally validate payload here
}

const char cmdTime[] = "millis"; // "time"
int idxSend = 0;

const int maxCalls = 2;
int cntCalls = 0;
const ulong msBetweenCalls = 5000;
ulong msLastExchangeEnded = 0;

void sendTimeRequest()
{
    Serial.println(String("sendTimeRequest @ ") + millis() + " ms, idx: " + idxSend);
    auto res = esp_now_send(++idxSend % 4 == 0 ? mac : macFail, (uint8_t *)cmdTime, strlen(cmdTime));
    Serial.println("ESP-NOW time request send result: " + String(res));
    retryer.onSendResult(res == 0);
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.println("Starting up...");
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    if (esp_now_init() != 0)
    {
        Serial.println("ESP-NOW init error");
        ledOnRestart();
    }
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    // auto res = esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    // if (res != 0)
    // {
    //     Serial.println("ESP-NOW add peer error: " + String(res));
    //     ledOnRestart();
    // }

    // for (int i = 0; i < 5; i++)
    // {
    //     sendTimeRequest(i);
    //     delay(3000);
    //     Serial.println();
    // }
}

void loop()
{
    // if (cntCalls < maxCalls && (msLastExchangeEnded == 0 || millis() >= msLastExchangeEnded + msBetweenCalls))
    if (cntCalls < maxCalls)
    {
        if (msLastExchangeEnded == 0 || millis() >= msLastExchangeEnded + msBetweenCalls)
        {
            if (retryer.readyToSend())
            {
                // auto err = esp_now_send(peerAddr, data, sizeof(data));
                sendTimeRequest();
            }
            if (retryer.stateEnd())
            {
                Serial.println(retryer.state() == Retryer::State::Success
                                   ? "Message exchange successful!\n"
                                   : "Exchange failed.\n");
                retryer.reset();
                msLastExchangeEnded = millis();
                cntCalls++;
            }
            retryer.update();
        }
    }
    else
    {
        Serial.println("Max calls reached, entering deep sleep...");
        ledOnDelay(1);
        ESP.deepSleep(0); // Sleep indefinitely, waiting for external reset to wake up and retry
    }
    delay(50);
}
