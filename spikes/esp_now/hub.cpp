//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

const byte pinRadioIn = 19;
const byte pinLed = 22; // LED_BUILTIN
bool isDataReceived = false;

char msg[10];

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
    // Serial.println("Data received!");
    // Serial.println(len);
    // Serial.println((char *)incomingData);
    isDataReceived = true;
    memcpy(msg, incomingData, len);
    Serial.println(atoi(msg));
}

void setup()
{
    pinMode(pinRadioIn, INPUT);
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, true);
    Serial.begin(115200);
    Serial.println("\n *** SensorNodeESP: HUB ***");
    // Serial.println("Receiving data...");
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (true)
            delay(100);
    }
#ifdef ESP32
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
#else
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
#endif
}

int cntKitchenSinkWater = 0; // counter for KitchenSinkWater pulses (5ms)
ulong msLastSignal = 0;      // time (msec) of the last received pulse
void pulseCount(ulong pulse, ulong ms)
{
    if (pulse > 4850 && pulse < 5000)
    {
        cntKitchenSinkWater++;
        msLastSignal = ms;
    }
}

static bool pulseCountOk(int cnt)
{
    return cnt >= 9 && cnt <= 11;
}

void loop()
{
    if (isDataReceived)
    {
        // digitalWrite(pinLed, false);
        // delay(500);
        // digitalWrite(pinLed, true);
        isDataReceived = false;
    }
    
    ulong pulse = pulseIn(pinRadioIn, LOW);
    ulong ms = millis();
    pulseCount(pulse, ms);    
    if (msLastSignal > 0 && ms > msLastSignal + 5)
    {
        if (pulseCountOk(cntKitchenSinkWater))
            Serial.println("*");
        cntKitchenSinkWater = 0;
        msLastSignal = 0;
    }
}
