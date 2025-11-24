//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04
//* ESP32 WROOM-32 test device (yellow pins) MAC Address: 30:AE:A4:47:9C:C4

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

#include "MacAddresses.h"
// uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x04};
uint8_t *mac = macSoftEsp32DevIpex;
bool sendSuccess = true;

#ifdef ESP32
esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}
#else
void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == 0) ? "Success" : "FAIL");
}
#endif

void printMAC(const uint8_t *mac)
{
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

ulong msLastRecv = 0;
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // printMAC(mac);
    Serial.printf("Recv data length: %d\n", len);
    if (len == 4)
    {
        ulong msHub;
        memcpy(&msHub, incomingData, len);
        Serial.println(msHub);
    }
    char s[10];
    memcpy(s, incomingData, len);
    s[len] = 0;
    Serial.println(s);
// e.g.
// Recv data length: 9
// 16:25:01

    // ulong msThisRecv = atol(s);
    // Serial.println(msThisRecv - msLastRecv);
    // msLastRecv = msThisRecv;
}

void setup()
{
    // pinMode(pinLed, OUTPUT);
    // ledOn(false);
    Serial.begin(115200);
    Serial.println();
    WiFi.mode(WIFI_STA);
    // Serial.println(WiFi.macAddress());
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (true)
            delay(100);
    }

    auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.printf("Register receiver code: %X\n", res);
#ifdef ESP32
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        while (true)
            delay(100);
    }
#else
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
#endif
}

char msg[] = "time";
ulong msLastSend = 0;

void loop()
{
    // strcpy(myData.a, "THIS IS A CHAR");
    // myData.b = random(1, 20);
    // myData.c = 1.2;
    // myData.d = false;
    // esp_err_t result = esp_now_send(mac, (uint8_t *)&myData, sizeof(myData));

    // itoa(cnt++, msg, 10);
    // ultoa(millis(), msg, 10);
    if (millis() > msLastSend + 6000)
    {
        Serial.println(msLastSend);
        Serial.println(millis());
        msLastSend = millis();
        esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
    }
    // delay(10);
}
