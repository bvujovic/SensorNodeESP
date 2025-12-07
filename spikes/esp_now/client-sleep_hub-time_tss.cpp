//* ESP-NOW Client that sleeps and wakes up to send data to Hub at specific time slots
//* This version uses TimeSlotSend class to manage time slots and wake-up timing

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

#include "TimeSlotSend.h"
TimeSlotSend tss(2, 10, 0, 200, 30);

#include "MacAddresses.h"
uint8_t *mac = macSoftEsp32DevIpex;
// bool sendSuccess = true;
// char cmdTime[] = "time";
// ulong msTimeToSendData = 0;

const byte pinLed = LED_BUILTIN;
void ledOn(bool on) { digitalWrite(pinLed, !on); }

// ulong msSendData = 0;        // Time to send data from sensors
// const byte maxRetries = 3; // Max retries for getting data from sensors

void sendTimeRequest()
{
    // int res;
    // byte timeRetries = 0;
    // do
    // {
    //     // printf("sendTimeRequest time: %lu\n", msLastSendReqTime);
    //     res = esp_now_send(mac, (uint8_t *)&cmdTime, strlen(cmdTime));
    //     if (res != 0)
    //     {
    //         printf("Initial esp_now_send res: 0x%X\n", res);
    //         ledOn(true);
    //         delay(10 * 1000);
    //         ledOn(false);
    //     }
    // } while (res != 0 && ++timeRetries < maxRetries);
    // if (timeRetries == maxRetries)
    //     msSendData = 1; // Force sending data without waiting for time response
    // esp_now_send(mac, (uint8_t *)&cmdTime, strlen(cmdTime));
    esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
    tss.timeReqIsSent(millis());
}

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
    // Serial.print("Last Packet Send Status: ");
    // Serial.println( == 0) ? "Success" : "FAIL");
    // if ((sendSuccess = sendStatus) != 0)
    if (sendStatus != 0)
        Serial.println("Last Packet Send Status: FAIL!!!");
}
#endif

void printMAC(const uint8_t *mac)
{
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// bool isWakeUpTimeOff = false;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    tss.onTimeStringRecv(incomingData, len, millis());
    // // if it's response to time command - e.g. 16:25:01
    // if (len == 8 && incomingData[2] == ':' && incomingData[5] == ':')
    // {
    //     char str[10];
    //     memcpy(str, incomingData, len);
    //     str[len] = 0;
    //     Serial.println(str);

    //     // auto secs = secondsUntilNextSlot(s, SLOT_MIN, SLOT_SEC);
    //     int h, m, s, secs = 0;
    //     if (sscanf(str, "%d:%d:%d", &h, &m, &s) != 3)
    //         secs = -1; // parsing error
    //     if (secs != -1)
    //         secs = tss.secondsUntilNextSlot(h, m, s);
    //     // Serial.println(secs);
    //     printf("Seconds to next %d-minute mark + %d sec: %d\n", tss.getSlotMin(), tss.getSlotSec(), secs);
    //     if (secs < 0)
    //     {
    //         Serial.println("Error!!!");
    //         sendTimeRequest();
    //     }
    //     else
    //         msTimeToSendData = millis() + secs * 1000UL - tss.getItvSensorRead();
    //     if (secs > tss.getSecWakeUpTimeOff()) // device woke up too early or too late
    //         isWakeUpTimeOff = true;
    // }
}

void setup()
{
    // TimeSlotSend *tc = new TimeSlotSend(2, 10, 0, 200, 30);
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.begin(115200);
    Serial.println();

    // Serial.println(secondsUntilNextSlot("16:25:00", 5, 5));  // 5
    // Serial.println(secondsUntilNextSlot("16:25:01", 2, 2));  // 61
    // Serial.println(secondsUntilNextSlot("16:29:15", 20, 0)); // 645
    // Serial.println(secondsUntilNextSlot("16:29:59", 30, 0)); // 1
    // Serial.println(secondsUntilNextSlot("16:29:59", 60, 0)); // 1801
    // Serial.println(secondsUntilNextSlot("16:30:00", 2, 0));  // 0
    // Serial.println(secondsUntilNextSlot("16:50:00", 10, 0)); // 0
    // Serial.println(secondsUntilNextSlot("16:55:30", 10, 1)); // 271
    // Serial.println(secondsUntilNextSlot("16:59:59", 2, 3));  // 4
    // Serial.println(secondsUntilNextSlot("16:28:00", 5, 10)); // 130
    // Serial.println(secondsUntilNextSlot("16:30:00", 5, 10)); // 10
    // Serial.println(secondsUntilNextSlot("16:30:10", 5, 10)); // 0
    // Serial.println(secondsUntilNextSlot("16:59:00", 5, 0));  // 60
    // Serial.println(secondsUntilNextSlot("23:59:55", 5, 2));  // 7
    // Serial.println(secondsUntilNextSlot("23:59:59", 5, 3));  // 4
    // Serial.println(secondsUntilNextSlot("00:00:00", 5, 4));  // 4
    // while (true)
    //     delay(100);

    WiFi.mode(WIFI_STA);
    // Serial.println(WiFi.macAddress());
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (true)
            delay(100);
    }

    auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    if (res != 0)
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
    // esp_now_send(mac, (uint8_t *)&cmdTime, strlen(cmdTime));
    sendTimeRequest();
}

void loop()
{
    if (tss.isTimeToSendData(millis()))
    {
        Serial.println("SEND DATA TO THE HUB");
        char msg[] = "data";
        auto res = esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
        if (res != 0) //? retry sending data?
        {
            printf("Data esp_now_send res: 0x%X\n", res);
            ledOn(true);
            delay(10 * 1000);
            ledOn(false);
        }
        delay(100); // wait for send callback
        Serial.println("GO TO SLEEP");
        // ESP.deepSleep((SLOT_MIN * 60 - SEC_BEFORE_WAKEUP) * 1000000UL);
        // ESP.deepSleep((tss.getSlotMin() * 60 - tss.getSecBeforeWakeup()) * 1000000UL);
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
