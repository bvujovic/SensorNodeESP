//* ESP32 hub/web server: collects data via ESP-NOW and SRX882 from sensors,
//* stores it on LittleFS and displays it on 192.168.0.80 in a web browser.
//* User can be notified with WhatsApp messages and/or buzzer.
//* Current consumption: ~150mA

#include <WiFi.h>
#include <Logger.h>
Logger logger;

#include <CredWiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> // lib_deps = esphome/ESPAsyncWebServer-esphome @ ^3.3.0
AsyncWebServer server(80);

#include <SrxParser.h>
SrxParser srx;
const byte pinRadioIn = 19;

#include "MyBlinky.h"
MyBlinky buzzer(18);

char line[80]; // general purpose char array - formating data
#include "my_esp_now.h"
#include "time.h"
struct tm ti;
ulong msLastGetTime = 0;
bool isTimeSet = false;

#include "TimeWatcher.h"
TimeWatcher tw(ti);

#include "NotifyWhatsApp.h"

void wifiConfig(bool isStaticIP)
{
    if (isStaticIP)
    {
        IPAddress ipa(192, 168, 0, 80);
        IPAddress gateway(192, 168, 0, 254);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.config(ipa, gateway, subnet);
    }
    else
        WiFi.config(IPAddress(), IPAddress(), IPAddress());
}

void startWebServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/ws/index.html", "text/html"); });
    server.on("/nodes.png", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(LittleFS, "/ws/nodes.png", "image/png"); });

    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        String list = req->arg("list");
        if (list == "dirs") // http://192.168.0.80/log?list=dirs
            req->send(200, "text/plain", logger.listFolders());
        if (list == "files") // http://192.168.0.80/log?list=files&dir=/2024_12
            req->send(200, "text/plain", logger.listFiles(req->arg("dir")));
        if (list == "file") // http://192.168.0.80/log?list=file&name=/2024_12/21_Sat.log
            req->send(200, "text/plain", logger.read(req->arg("name"))); });

    server.on("/sensorTypeComment", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        const char *sensor = req->arg("sensor").c_str(); // http://192.168.0.80/sensorTypeComment?sensor=EnsAht
        int sensorType = SensorType::UndefinedSensorType;
        int n = sizeof(StrSensorTypes) / sizeof(char *);
        for (size_t i = 0; i < n; i++)
            if (strcmp(StrSensorTypes[i], sensor) == 0)
                sensorType = i;
        req->send(200, "text/plain", SensorTypesComment[sensorType]); });

    server.on("/nots", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        if (req->hasArg("id")) // http://192.168.0.80/nots?id=B1&val=1
        {
            int id = req->arg("id").substring(1).toInt();
            int val = req->arg("val").toInt();
            for (auto &&n : notifications)
                if (n.id == id)
                {
                    if (req->arg("id")[0] == 'B')
                        n.buzz = val;
                    else
                        n.wa_msg = val;
                }
            req->send(200, "text/plain", "");
        }
        else // http://192.168.0.80/nots
        {
            // TODO use sprintf()
            String s;
            for (auto &&n : notifications)
            {
                // Serial.printf("%d, %s, %d, %d \n", n.id, n.name.c_str(), n.buzz, n.wa_msg);
                sprintf(line, "%d\t%s\t%d\t%d\n", n.id, n.name.c_str(), n.buzz, n.wa_msg);
                s += line;
            }
            req->send(200, "text/plain", s);
        } });

    server.on("/buzzOnMinGet", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(200, "text/plain", String(tw.getIsItOn() ? tw.getBuzzOnMin() : 0)); });
    server.on("/buzzOnMinSave", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        req->send(200, "text/plain", "");
        int min = req->arg("min").toInt(); // http://192.168.0.80/buzzOnMinSave?min=10
        tw.setBuzzOnMin(min); });

    server.on("/statusInfo", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        getLocalTime(&ti);
        strftime(line, sizeof(line), "%Y-%m-%d %H:%M:%S\n", &ti);
        int percHeap = 100 * ESP.getFreeHeap() / ESP.getHeapSize();
        size_t freeStorage = LittleFS.totalBytes() - LittleFS.usedBytes();
        int percStorage = 100 * freeStorage / LittleFS.totalBytes();
        String s = String("Current time: ") + line \
        + "Heap: free " + (ESP.getFreeHeap() / 1024) + " KB / " + (ESP.getHeapSize() / 1024) + " KB total (" + percHeap + "%)\n" \
        + "Storage: free " + (freeStorage / 1024) + " KB / " + (LittleFS.totalBytes() / 1024) + " KB total (" + percStorage + "%)\n";
        s.replace("\n", "<br>");
        req->send(200, "text/plain", s); });

    server.begin();
}

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

    startWebServer();

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
}

void loop()
{
    // Parsing signals from simple sensors (PIR, water detection...) with SRX882
    SrxCommand cmd = srx.refresh(pulseIn(pinRadioIn, LOW), millis());
    if (cmd != None)
    {
        Serial.printf("SRX882: %d\n", cmd);
        if (notifications[WaterDetected].wa_msg)
        {
            wifiConfig(false);
            delay(3000);
            // 💥Stan, kuhinja, sudopera:
            // VISOK NIVO VODE U SUDOPERI 💦
            NotifyWhatsApp::sendMessage("%F0%9F%92%A5+Stan,+kuhinja,+sudopera:%0AVISOK+NIVO+VODE+U+SUDOPERI!+%F0%9F%92%A6");
            wifiConfig(true);
        }
        if (notifications[WaterDetected].buzz)
            buzzer.blinkCritical();
        logger.add(StrSensorTypes[SensorType::SimpleEvent], "KitchenSinkWater", "Water detected!");
    }
    // ESP-NOW: reply to "millis" command
    // TODO try to move this code to ESP-NOW:Receive method
    if (peerRespMillis != NULL)
    {
        // TODO vratiti odgovor samo ako je millis() < 4mlrd ili tako nesto - da se ne bi desio overflow u toku merenja vremena
        ulong ms = millis();
        ultoa(ms, line, 10);
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
            wifiConfig(true);
            Serial.println(WiFi.localIP());
            // logger.add("HUB", "HUB", "got time");
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
