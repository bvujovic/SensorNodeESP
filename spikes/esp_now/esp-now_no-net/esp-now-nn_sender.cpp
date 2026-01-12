#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "MacAddresses.h"
#include <esp_wifi.h>

const byte pinLed = 22; // builtin LED on ESP32 w/ battery connector
void ledOn(bool on) { digitalWrite(pinLed, !on); }
void ledOnError(const char *message)
{
    Serial.println(message);
    for (size_t i = 0;; i++)
    {
        ledOn(i % 2);
        delay(1000);
    }
}

uint8_t *mac = macEsp32Dev;
bool sendSuccess = false;

esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}

void setup()
{
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.begin(115200);
    Serial.println();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != 0)
        ledOnError("ESP-NOW init fail!");

    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
        ledOnError("Failed to add ESP-NOW peer!");
}

char msg[10];
int cnt = 0;

void loop()
{
    itoa(cnt++, msg, 10);
    auto res = esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
    Serial.printf("Send response: 0x%X\n", res);
    delay(4000);

    ledOn(!sendSuccess);
    delay(1000);
    ledOn(false);
}
