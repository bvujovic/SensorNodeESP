#include <esp_now.h>
#include <WiFi.h>
#include "MacAddresses.h"
#include <esp_wifi.h>

const byte pinLed = 1; // builtin LED on ESP32 Dev module - Serial will not work!
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

uint8_t *macClient = macEsp32BattConn;
bool isDataReceived;
esp_now_peer_info_t peerInfo;

void printMAC(const uint8_t *mac)
{
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

esp_err_t addPeer()
{
    memcpy(peerInfo.peer_addr, macClient, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    auto res = esp_now_add_peer(&peerInfo);
    if (res != ESP_OK)
        Serial.printf("Failed to add peer: %X\n", res);
    return res;
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // printMAC(mac); memcpy(macClient, mac, 6);
    char msg[80];
    memcpy(msg, incomingData, len);
    msg[len] = 0;
    Serial.println(msg);
    isDataReceived = true;
}

void setup()
{
    Serial.begin(115200); // Serial will not work because of pinLed
    Serial.println("\nServer started!");
    pinMode(pinLed, OUTPUT);
    ledOn(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != ESP_OK)
        ledOnError("ESP-NOW init fail!");

    // uint8_t testMac[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    memcpy(peerInfo.peer_addr, macClient, 6);
    peerInfo.encrypt = peerInfo.channel = 0;
    auto res = esp_now_add_peer(&peerInfo);
    if (res != ESP_OK)
        ledOnError(("Failed to add peer, res: " + String(res)).c_str());
    else
        Serial.println("Client peer added.");
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.printf("Setup ms: %lu\n", millis());
    ledOn(false);
}

void loop()
{
    if (isDataReceived)
    {
        isDataReceived = false;
        ledOn(true);
        printMAC(macClient);
        ulong ms = millis();
        Serial.println(ms);
        // esp_now_send(macClient, (uint8_t *)&msg, strlen(msg));
        delay(500);
        ledOn(false);
    }
    delay(10);
}
