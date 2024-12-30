#include <WiFi.h>
#include <Logger.h>
Logger logger;
#include <WiFi.h>
#include <CredWiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> // lib_deps = esphome/ESPAsyncWebServer-esphome @ ^3.3.0
AsyncWebServer server(80);

#include <SrxParser.h>
SrxParser srx;
const byte pinRadioIn = 19;

#include "MyBlinky.h"
MyBlinky buzzer(18);

#include "my_esp_now.h"
#include "time.h"
struct tm ti;
ulong msLastGetTime = 0;
bool isTimeSet = false;

#include "TimeWatcher.h"
TimeWatcher tw(ti);

void setup()
{
    Serial.begin(115200);
    Serial.println("\n*** SensorNodeESP: HUB ***");
    pinMode(pinRadioIn, INPUT);
    LittleFS.begin();
    logger.setTimeInfo(ti);
    tw.setBlinky(buzzer.getBlinky());

    // WiFi
    WiFi.mode(WIFI_AP_STA);
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
    // Web Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       Serial.println("ESP32 Web Server: New request (/)");
       request->send(200, "text/html", "<html><body><h1>Hello, ESP32!</h1></body></html>"); });
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req)
              {
                  Serial.println("Web Server: New request (/log)");
                  req->send(200, "text/html", "<html><body><h1>SensorNodeESP HUB: Web server</h1></body></html>");
                  String list = req->arg("list");
                  if (list == "dirs") // http://192.168.0.80/log?list=dirs
                      Serial.println(logger.listFolders());
                  if (list == "files") // http://192.168.0.80/log?list=files&dir=/2024_12
                      Serial.println(logger.listFiles(req->arg("dir")));
                  if (list == "file") // http://192.168.0.80/log?list=file&name=/2024_12/21_Sat.log
                      Serial.println(logger.read(req->arg("name"))); });
    server.on("/buzzOnMin", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        Serial.println("Web Server: New request (/buzzOnMin)");
        req->send(200, "text/html", "<html><body><h1>SensorNodeESP HUB: Web server</h1></body></html>");
        int min = req->arg("min").toInt(); // http://192.168.0.80/buzzOnMin?min=10
        tw.setBuzzOnMin(min);
        if (tw.getIsItOn())
            Serial.printf("TimeWatcher: buzzOnMin = %u\n", tw.getBuzzOnMin());
        else
            Serial.println("TimeWatcher is OFF");
        Serial.printf("Heap: free %u KB / %u KB total\n", ESP.getFreeHeap() / 1024, ESP.getHeapSize() / 1024); });
    server.begin();

    // ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        while (true)
            delay(100);
    }
    setPeers();
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    esp_now_register_send_cb(OnDataSent);

    // B tw.test();
}

char msg[10];

void loop()
{
    // Parsing signals from simple sensors (PIR, water detection...) with SRX882
    SrxCommand cmd = srx.refresh(pulseIn(pinRadioIn, LOW), millis());
    if (cmd != None)
    {
        Serial.printf("SRX882: %d\n", cmd);
        buzzer.blinkCritical();
        logger.add(StrSensorTypes[SensorType::SimpleEvent], "KitchenSinkWater", "Water detected!");
    }
    // ESP-NOW: reply to "millis" command
    // TODO try to move this code to ESP-NOW:Receive method
    if (peerRespMillis != NULL)
    {
        ulong ms = millis();
        ultoa(ms, msg, 10);
        esp_now_send(peerRespMillis->peer_addr, (uint8_t *)&ms, 4);
        peerRespMillis = NULL;
    }

    if (isTimeSet)
    {
        if (WiFi.localIP()[3] == 80) // if time and IP address are set...
        {
            getLocalTime(&ti);
            tw.buzzIN();
        }
        else
        {
            // set up IP address after current date/time is retrieved
            IPAddress ipa(192, 168, 0, 80);
            IPAddress gateway(192, 168, 0, 254);
            IPAddress subnet(255, 255, 255, 0);
            WiFi.config(ipa, gateway, subnet);
            Serial.println(WiFi.localIP());
            logger.add("HUB", "HUB", "got time");
        }
    }
    else if (millis() > msLastGetTime + 1000)
    // current time test
    {
        msLastGetTime = millis();
        if (getLocalTime(&ti))
        {
            Serial.println(&ti, "getLocalTime: %H:%M:%S");
            isTimeSet = true;
        }
        else
            Serial.println("getLocalTime fail!");
    }
}
