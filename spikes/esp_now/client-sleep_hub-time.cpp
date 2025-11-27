//* ESP-NOW Client that sleeps and wakes up to send data to Hub at specific time slots
//* The client requests current time from the Hub, calculates time to the next n-minute mark
//* plus TIME_SLOT seconds, sends data at that time, then goes to sleep for SLEEP_TIME minutes.

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

#define TIME_SLOT (5)    // seconds Data sending time slot after the 10-minute mark: 11:00:05, 11:10:05...
#define MARK_MIN (2)     // minutes
#define SLEEP_TIME (1.9) // minutes
//* sleep time should be less than MARK_MIN to wake up before next mark

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

    // // If 60 minutes: next mark is top of next hour
    // if (nextMarkMin == 60)
    //     nextMarkSec = 60 * 60; // 3600 seconds

    auto res = nextMarkSec - secNow;
    return res != markMin * 60 ? res : 0;
}
// Tests for secondsToNextMark()
// Serial.println(secondsToNextMark("16:25:00", 5));
// Serial.println(secondsToNextMark("16:25:01", 2));
// Serial.println(secondsToNextMark("16:29:15", 20));
// Serial.println(secondsToNextMark("16:29:59", 30));
// Serial.println(secondsToNextMark("16:29:59", 60));
// Serial.println(secondsToNextMark("16:30:00", 2));
// Serial.println(secondsToNextMark("16:50:00", 10));
// Serial.println(secondsToNextMark("16:55:30", 10));
// Serial.println(secondsToNextMark("16:59:59", 2));

#include "MacAddresses.h"
uint8_t *mac = macSoftEsp32DevIpex;
bool sendSuccess = true;
char cmdTime[] = "time";
ulong msTimeToSendData = 0;

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

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // if it's response to time command - e.g. 16:25:01
    if (len == 8 && incomingData[2] == ':' && incomingData[5] == ':')
    {
        char s[10];
        memcpy(s, incomingData, len);
        s[len] = 0;
        Serial.println(s);

        auto secs = secondsToNextMark(s, MARK_MIN);
        Serial.println(secs);
        if (secs < 0)
            Serial.println("Error!!!"); // TODO send data immediately or send time command again
        else
            msTimeToSendData = millis() + (secs + TIME_SLOT) * 1000UL;
    }
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

    esp_now_send(mac, (uint8_t *)&cmdTime, strlen(cmdTime));
}

void loop()
{
    if (msTimeToSendData != 0 && millis() > msTimeToSendData)
    {
        Serial.println("SEND DATA TO THE HUB");
        char msg[] = "data";
        esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
        delay(100); // wait for send callback
        Serial.println("GO TO SLEEP");
        ESP.deepSleep(1000000UL * 60 * SLEEP_TIME);
    }
    else
    {
        // Serial.print('.');
        delay(250);
    }
}
