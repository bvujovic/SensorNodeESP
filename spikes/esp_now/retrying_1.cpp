//* Send couple of times "time" command to the hub. Every other time send that command to 
//* the wrong MAC address.

#include "Arduino.h"
#define SECOND (1000UL)
#define MINUTE (60 * SECOND)

// using Action = std::function<bool(void)>;
// using ActionVoid = std::function<void(void)>;

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

void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    Serial.println("OnDataSent status: " + String(sendStatus));
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    Serial.println("OnDataRecv: " + String((const char *)incomingData));
}

const char cmdTime[] = "time";

void sendTimeRequest(int idx)
{
    Serial.println(String("sendTimeRequest @ ") + millis() + " ms, idx: " + idx);
    auto res = esp_now_send(idx % 2 == 0 ? mac : macFail, (uint8_t *)cmdTime, strlen(cmdTime));
    Serial.println("ESP-NOW time request send result: " + String(res));
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

    for (int i = 0; i < 5; i++)
    {
        sendTimeRequest(i);
        delay(3000);
        Serial.println();
    }

    ESP.deepSleep(0); // Sleep indefinitely, waiting for external reset to wake up and retry
}

void loop()
{
    delay(1000);
}