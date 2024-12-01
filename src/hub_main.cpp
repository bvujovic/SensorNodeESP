#include <WiFi.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <CredWiFi.h>
#include <AsyncTCP.h>
// lib_deps = esphome/ESPAsyncWebServer-esphome @ ^3.3.0
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

#include <SrxParser.h>
SrxParser srx;
const byte pinRadioIn = 19;

#include "my_esp_now.h"

void setup()
{
    Serial.begin(115200);
    Serial.println("\n*** SensorNodeESP: HUB ***");
    pinMode(pinRadioIn, INPUT);
    LittleFS.begin();

    // WiFi
    WiFi.mode(WIFI_AP_STA);
    //? WiFi.persistent(false);
    // WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Serial.print("Connecting to WiFi");
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     Serial.print('.');
    //     delay(1000);
    // }
    // // TODO odredjivanje IP adrese ili naziva servera, npr SetupIPAddress(100);
    // Serial.println(" connected.");
    // Serial.print("ESP32 Web Server's IP address: ");
    // Serial.println(WiFi.localIP());
    // // Web Server
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {
    //    Serial.println("ESP32 Web Server: New request received:");  // for debugging
    //    request->send(200, "text/html", "<html><body><h1>Hello, ESP32!</h1></body></html>"); });
    // server.begin();

    // ESP-NOW
    setPeers();
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        while (true)
            delay(100);
    }
    esp_now_register_send_cb(OnDataSent);

    // uint8_t testMac[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    //*memcpy(peerInfo.peer_addr, macEsp32Dev, 6);
    //*peerInfo.encrypt = peerInfo.channel = 0;
    // peerInfo.channel = 0;
    // peerInfo.encrypt = false;
    //*auto res = esp_now_add_peer(&peerInfo);

    // auto res = esp_now_add_peer(&peers[0]);
    Serial.println("Adding ESP-NOW peers: ");
    for (auto &&p : peers)
    {
        auto res = esp_now_add_peer(&p);
        if (res != ESP_OK)
        {
            Serial.println("- Failed to add peer");
            Serial.printf("- Reason: %X\n", res);
        }
        else
            Serial.printf("\t%s\n", p.lmk);
            // Serial.println((char *)p.lmk);
    }
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

char msg[10];

void loop()
{
    // Parsing signals from simple sensors (PIR, water detection...) with SRX882
    SrxCommand cmd = srx.refresh(pulseIn(pinRadioIn, LOW), millis());
    if (cmd != None)
    {
        Serial.printf("SRX882: %d\n", cmd);
    }
    // ESP-NOW
    if (peerRespMillis != NULL)
    {
        Serial.print("Send to: ");
        Serial.println((char *)peerRespMillis->lmk);
        ulong ms = millis();
        ultoa(ms, msg, 10);
        // esp_now_send(peerRespMillis->peer_addr, (uint8_t *)&msg, strlen(msg));
        esp_now_send(peerRespMillis->peer_addr, (uint8_t *)&ms, 4);
        peerRespMillis = NULL;
    }

    // if (millis() > msServerStarted + 30000)
    // {
    //     msServerStarted = INT32_MAX;
    //     server.end();
    //     Serial.println("WebServer stopped.");
    // }
}
