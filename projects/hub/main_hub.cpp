//* ESP32 hub/web server: collects data via ESP-NOW from sensors,
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

#include "MyBlinky.h"
MyBlinky buzzer(18);

#define SECOND (1000UL)
#define MY_NTP_SERVER "rs.pool.ntp.org"
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Europe/Belgrade -> CET-1CEST,M3.5.0,M10.5.0/3
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#include "time.h"
#include "esp_sntp.h"
time_t now; // this are the seconds since Epoch (1970) - UTC
struct tm ti;
char line[80]; // general purpose char array - formating data
#include "NotifyWhatsApp.h"
// R bool isSimpleEventHandled = false;
#include "SimpleEventHandler.h"
SimpleEventHandler seh;
#include "Enums.h"
#include "my_esp_now.h"
// ulong msLastGetTime = 0;
// bool isTimeSet = false;

void getTime()
{
    time(&now);             // read the current time
    localtime_r(&now, &ti); // update the structure tm with the current time
}

ulong msLastTimeSync = 0;
// callback function to show when NTP was synchronized
void cbSyncTime(struct timeval *tv)
{
    // Serial.println(F(" *** NTP time synched! *** "));
    msLastTimeSync = millis();
    // Serial.println(msLastTimeSync);
}

#include "TimeWatcher.h"
TimeWatcher tw(ti);

const byte lastIpNumber = 80; // last byte of IP address for static IP assignment
void wifiConfig(bool isStaticIP)
{
    if (isStaticIP)
    {
        IPAddress ipa(192, 168, 0, lastIpNumber);
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

    server.serveStatic("/img/", LittleFS, "/ws/img/")
        .setCacheControl("max-age=604800, public, immutable"); // cache for 7 days

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
        // int n = sizeof(ToString::StrSensorTypes) / sizeof(char *);
        // int n = sizeof(SensorType::SensorTypeCount) / sizeof(char *);
        // int n = SensorType::SensorTypeCount;
        for (size_t i = 0; i < SensorType::SensorTypeCount; i++)
            if (strcmp(ToString::SensorTypes[i], sensor) == 0)
                sensorType = i;
        req->send(200, "text/plain", ToString::SensorTypesComment[sensorType]); });

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
            String s;
            for (auto &&n : notifications)
            {
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
        auto heapSize = ESP.getHeapSize();
        auto usedHeap = heapSize - ESP.getFreeHeap();
        int percHeap = (100.0 * usedHeap / heapSize) + 0.5;
        auto storageTotal = LittleFS.totalBytes();
        // auto percStorage = 100 * LittleFS.usedBytes() / storageTotal;
        auto usedStorage = LittleFS.usedBytes();
        int percStorage = (100.0 * usedStorage / storageTotal) + 0.5;
        auto s = String("Current time: ") + line \
        + "Heap: used " + (usedHeap / 1024) + " KB / " + (heapSize / 1024) + " KB total (" + percHeap + "%)\n" \
        + "Storage: used " + (usedStorage / 1024) + " KB / " + (storageTotal / 1024) + " KB total (" + percStorage + "%)\n";
        s.replace("\n", "<br>");
        req->send(200, "text/plain", s); });

    server.on("/removeDir", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        auto dir = req->arg("dir");
        req->send(200, "text/plain", logger.removeFolder(dir) ? "1" : "0"); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        req->send(200, "text/plain", "Reset started");
        ESP.restart(); });

    server.begin();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n*** SensorNodeESP: HUB ***");
    LittleFS.begin();
    logger.setTimeInfo(ti);
    tw.setBlinky(buzzer.getBlinky());

    // WiFi
    WiFi.mode(WIFI_AP_STA); // ESP32 has to be in this mode to be able to use ESP-NOW and Web Server at the same time
    //? WiFi.persistent(false);
    WiFi.softAP("ESP_Hub", "SomeDumbPa$$22", 1, true); // hidden SSID
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    Serial.println(" connected.");
    // Serial.print("ESP32 Web Server's IP address: ");
    // Serial.println(WiFi.localIP());

    sntp_set_sync_interval(7 * 24 * 60 * 60 * SECOND); // sync every week (daily auto reset will update time once a day)
    sntp_set_time_sync_notification_cb(cbSyncTime);
    configTime(0, 0, MY_NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", MY_TZ, 1);          // Set environment variable with your time zone
    tzset();
    Serial.print("Waiting for NTP time sync: ");
    while (msLastTimeSync == 0)
        delay(200);
    Serial.println("Time synchronized!");
    // Serial.println(msLastTimeSync);

    wifiConfig(true);
    startWebServer();
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

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

String message;

void loop()
{
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
    // ESP-NOW: handle Simple Event messages
    if (seh.isNewMessageReceived())
    {
        // Serial.printf("Simple Event received from %s: %s\n", seh.getDeviceName(), seh.getMessageText());
        auto peer = seh.getPeerInfo();
        if (peer->device == Device::ESP32BattConn)
        {
            auto notif = GetNotif(WaterDetected);
            if (notif != NULL)
            {
                if (notif->wa_msg)
                {
                    // Serial.println("Sending WhatsApp message about water detected...");
                    wifiConfig(false);
                    delay(3000);
                    // 💥Stan, kuhinja, sudopera:
                    // VISOK NIVO VODE U SUDOPERI 💦
                    auto res = NotifyWhatsApp::sendMessage("%F0%9F%92%A5+Stan,+kuhinja,+sudopera:%0AVISOK+NIVO+VODE+U+SUDOPERI!+%F0%9F%92%A6");
                    if (res != 200)
                        logger.add("NotifyWhatsApp", "ESP32Hub", (String("WhatsApp message sent, resp code: ") + res).c_str());
                    wifiConfig(true);
                }
                if (notif->buzz)
                    buzzer.blinkCritical();
            }
            logger.add(ToString::SensorTypes[peer->type], ToString::Devices[peer->device], seh.getMessageText());
        }
        seh.clearEventData();
    }

    getLocalTime(&ti);
    tw.buzzIN();
    if (ti.tm_hour == 22 && ti.tm_min == 22 && ti.tm_sec == 22)
        ESP.restart();

    delay(10);
}
