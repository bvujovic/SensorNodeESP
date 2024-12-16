#include <WiFi.h>
#include "Logger.h"
Logger logger;

#include <WiFi.h>
#include <CredWiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> // lib_deps = esphome/ESPAsyncWebServer-esphome @ ^3.3.0
AsyncWebServer server(80);

#include <SrxParser.h>
SrxParser srx;
const byte pinRadioIn = 19;

const byte pinLed = 22;

#include "my_esp_now.h"
struct tm ti;

//* My NodeMCU's MAC address: 84:F3:EB:77:04:BA
uint8_t mac[] = {0x84, 0xF3, 0xEB, 0x77, 0x04, 0xBA};

void setup()
{
    Serial.begin(115200);
    Serial.println("\n*** SensorNodeESP: HUB ***");
    pinMode(pinRadioIn, INPUT);
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, true);
    LittleFS.begin();
    logger.setTimeInfo(ti);

    // WiFi
    WiFi.mode(WIFI_AP_STA);
    // WiFi.mode(WIFI_STA);
    //? WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(" connected.");
    Serial.print("ESP32 Web Server's IP address: ");
    Serial.println(WiFi.localIP());
    configTime(3600, 3600, "rs.pool.ntp.org");
    // WiFi.disconnect();

    //? logger.setTimeInfo(ti);
    // Serial.printf("Total space: %u KB\n", logger.getTotalKB());
    // Serial.printf("Used space: %u KB\n", logger.getUsedKB());
    // auto msgLogged = logger.add("test", "test poruka");
    // Serial.println(msgLogged ? "Test msg added to log file." : "Failed to log test msg.");
    // Serial.println("Folders:");
    // Serial.println(logger.listFolders());
    // Serial.println("Files in /2024_12:");
    // Serial.println(logger.listFiles("/2024_12"));
    // Serial.println("File content for /2024_12/05_Thu.log:");
    // Serial.println(logger.read("/2024_12/05_Thu.log"));
    // WiFi.disconnect(true);

    // IPAddress ipa(192, 168, 0, 80);
    // IPAddress gateway(192, 168, 0, 254);
    // IPAddress subnet(255, 255, 255, 0);
    // WiFi.config(ipa, gateway, subnet);
    // Serial.println(WiFi.localIP());

    // // Web Server
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    //           {
    //    Serial.println("ESP32 Web Server: New request received:");  // for debugging
    //    request->send(200, "text/html", "<html><body><h1>Hello, ESP32!</h1></body></html>"); });
    // server.begin();

    // ESP-NOW
    // setPeers();
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        while (true)
            delay(100);
    }
    // esp_now_register_send_cb(OnDataSent);
    // Serial.println("Adding ESP-NOW peers: ");
    // for (auto &&p : peers)
    // {
    //     auto res = esp_now_add_peer(&p);
    //     if (res != ESP_OK)
    //     {
    //         Serial.println("- Failed to add peer");
    //         Serial.printf("- Reason: %X\n", res);
    //     }
    //     else
    //         Serial.printf("\t%s\n", p.lmk);
    // }
    // memcpy(peerInfo.peer_addr, mac, 6);
    // peerInfo.channel = 1;
    // peerInfo.encrypt = false;
    // if (esp_now_add_peer(&peerInfo) != ESP_OK)
    // {
    //     Serial.println("Failed to add peer");
    //     return;
    // }
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

char msg[10];

void loop()
{
    // if (getLocalTime(&ti))
    // {
    //     digitalWrite(pinLed, !(ti.tm_min % 10 == 0 && ti.tm_sec == 0));
    // }
    // delay(1000);

    // Parsing signals from simple sensors (PIR, water detection...) with SRX882
    SrxCommand cmd = srx.refresh(pulseIn(pinRadioIn, LOW), millis());
    if (cmd != None)
    {
        Serial.printf("SRX882: %d\n", cmd);
        if (cmd == KitchenSinkWater)
            logger.add("kujna/sudopera", "Visok nivo vode u sudoperi!");
    }
    // ESP-NOW
    if (peerRespMillis != NULL)
    {
        Serial.print("Send to: ");
        Serial.println((char *)peerRespMillis->lmk);
        ulong ms = millis();
        // ultoa(ms, msg, 10);
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
