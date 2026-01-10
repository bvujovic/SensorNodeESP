//* ESP-NOW burst messages sender - Sends a few messages and goes to deep sleep

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "MacAddresses.h"

const byte pinLed = 2; // LED_BUILTIN
void ledOn(bool on) { digitalWrite(pinLed, !on); }

// uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x04};
uint8_t *mac = macEsp32DevIpex;
bool sendSuccess = true;

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
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (true)
            delay(100);
    }

    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        ledOn(true);
        while (true)
            delay(100);
    }
}

// B char msg[] = "Pozdrav ESP-Now";
char msg[10];
int cnt = 0;

void loop()
{
    if (cnt++ > 5)
        esp_deep_sleep_start();
    // while (true)
    //     delay(1000);

    ultoa(millis(), msg, 10);
    auto res = esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
    Serial.printf("Send response: 0x%X\n", res);
    ledOn(true);
    delay(500);
    ledOn(false);

    // ledOn(!sendSuccess);
    // delay(1000);
    // ledOn(false);
}
