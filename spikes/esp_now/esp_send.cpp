//* https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/
//* https://randomnerdtutorials.com/esp-now-esp8266-nodemcu-arduino-ide/

#include <Arduino.h>
#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

const byte pinLed = 2; // LED_BUILTIN
void ledOn(bool on) { digitalWrite(pinLed, !on); }

// CC:50:E3:0F:62:84
// uint8_t mac[] = {0xCC, 0x50, 0xE3, 0x0F, 0x62, 0x84};
// uint8_t mac[] = {0x84, 0xF3, 0xEB, 0x77, 0x04, 0xBA};
// 30:C6:F7:04:66:04
uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x04};
esp_now_peer_info_t peerInfo;
bool sendSuccess = true;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}

void setup()
{
    pinMode(pinLed, OUTPUT);
    // digitalWrite(pinLed, true);
    ledOn(false);
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

#ifdef ESP32
    esp_now_register_send_cb(OnDataSent);
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
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
#endif
}

// B char msg[] = "Pozdrav ESP-Now";
char msg[10];
int cnt = 0;

// typedef struct struct_message
// {
//     char a[32];
//     int b;
//     float c;
//     bool d;
// } struct_message;
// struct_message myData;

void loop()
{
    // strcpy(myData.a, "THIS IS A CHAR");
    // myData.b = random(1, 20);
    // myData.c = 1.2;
    // myData.d = false;
    // esp_err_t result = esp_now_send(mac, (uint8_t *)&myData, sizeof(myData));

    itoa(cnt++, msg, 10);
    esp_now_send(mac, (uint8_t *)&msg, strlen(msg));
    delay(4000);

    ledOn(!sendSuccess);
    delay(1000);
    ledOn(false);
}
