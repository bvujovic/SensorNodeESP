//* ESP32 hub/web server: collects data via ESP-NOW from sensors,
//* stores it on LittleFS and displays it on 192.168.0.80 in a web browser.
//* User can be notified with WhatsApp messages and/or buzzer.
//* Current consumption: ~150mA

#include <WiFi.h>
#include <Logger.h>
Logger logger;

// // #define BANOVO_BRDO
// #define VRANIC
#include "Enums.h"

#if defined(BANOVO_BRDO)
#include <CredWiFi_Vujovic.h>
#elif defined(VRANIC)
#include <CredWiFi_Vranic.h>
#endif
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> // lib_deps = esphome/ESPAsyncWebServer-esphome @ ^3.3.0
AsyncWebServer server(80);

#include "MyBlinky.h"
#if defined(BANOVO_BRDO)
MyBlinky buzzer(18);
#elif defined(VRANIC)
MyBlinky buzzer(14);
#endif

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
#include "SimpleEventHandler.h"
SimpleEventHandler seh;
#include "Enums.h"
#include "my_esp_now.h"

void getTime()
{
  time(&now);             // read the current time
  localtime_r(&now, &ti); // update the structure tm with the current time
}

ulong msLastTimeSync = 0;
// callback function to show when NTP was synchronized
void cbSyncTime(struct timeval *tv)
{
  Serial.printf("NTP time synched! %lu \n", msLastTimeSync = millis());
  // msLastTimeSync = millis();
  // Serial.println(msLastTimeSync);
}

#include "TimeWatcher.h"
TimeWatcher tw(ti);

const byte lastIpNumber = 80; // last byte of IP address for static IP assignment
void wifiConfig(bool isStaticIP)
{
  if (isStaticIP)
  {
    // WiFi.channel(1); // set channel to 1, so ESP-NOW and WiFi AP are on the same channel

    // #if defined(BANOVO_BRDO)
    //     IPAddress local_ip(192, 168, 0, lastIpNumber);
    //     IPAddress gateway(192, 168, 0, 254);
    // #elif defined(VRANIC)
    //     IPAddress local_ip(192, 168, 1, lastIpNumber);
    //     IPAddress gateway(192, 168, 1, 254);
    // #endif
    //     IPAddress subnet(255, 255, 255, 0);

    //     // IPAddress dns1(8, 8, 8, 8);
    //     // IPAddress dns2(8, 8, 4, 4);
    //     //* Change these to use your gateway directly for DNS resolution
    //     IPAddress dns1 = gateway;
    //     IPAddress dns2(8, 8, 8, 8); // Keep Google as a backup secondary

#if defined(BANOVO_BRDO)
    IPAddress local_ip(192, 168, 0, lastIpNumber);
    IPAddress gateway(192, 168, 0, 1); // Changed from 254 to 1
#elif defined(VRANIC)
    // IPAddress local_ip(192, 168, 1, lastIpNumber);
    // IPAddress gateway(192, 168, 1, 1); // Changed from 254 to 1
    IPAddress local_ip(192, 168, 8, lastIpNumber);
    IPAddress gateway(192, 168, 8, 1); // Changed from 254 to 1
#endif
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(8, 8, 8, 8); // Primary public DNS
    IPAddress dns2(1, 1, 1, 1); // Secondary public DNS

    WiFi.config(local_ip, gateway, subnet, dns1, dns2);
  }
  else
    WiFi.config(IPAddress(), IPAddress(), IPAddress(), IPAddress(), IPAddress());
  // WiFi.channel(1); // set channel to 1, so ESP-NOW and WiFi AP are on the same channel
}

// #include <WiFiClientSecure.h>
// #include <PubSubClient.h> // lib_deps = knolleary/PubSubClient @ ^2.8
// #include "azure-secrets.h"
#include "ActionNodes.h"
ActionNodes actionNodes(logger); // Create an instance of ActionNodes and pass the logger reference

// // Azure Configuration Details
// const char *mqtt_server = SECRET_MQTT_SERVER;
// const int mqtt_port = 8883;
// const char *client_id = SECRET_DEVICE_ID; // Must match Azure Device ID exactly

// // Username format MUST be exactly this:
// const char *mqtt_username = SECRET_MQTT_USER;

// // Paste your entire 1-year SAS token here:
// const char *mqtt_password = SECRET_SAS_TOKEN;

// // The Azure topic for Cloud-to-Device messages
// const char *c2d_topic = "devices/TestSensorHub/messages/devicebound/#";

// WiFiClientSecure azureSecureClient;
// PubSubClient azureMqttClient(azureSecureClient);

// // Remote Node Wake Queue Variables
// bool hasPendingCommand = false;
// uint8_t pendingCommandPayload = 0;

// // 1. Handle incoming commands sent from the Azure Cloud
// void azureCallback(char *topic, byte *payload, unsigned int length)
// {
//   String s = "azureCallback";

//   Serial.print("Cloud message arrived on topic: ");
//   Serial.println(topic);

//   if (length > 0)
//   {
//     // Capture the command byte (e.g. '1', '2', etc.)
//     pendingCommandPayload = payload[0];
//     hasPendingCommand = true;
//     Serial.printf("Command buffered for battery node: %c\n", pendingCommandPayload);
//     s += String(" - Command buffered for battery node: ") + (char)pendingCommandPayload;
//     logger.add("Azure", "HUB", s.c_str());
//   }
// }

// // 2. Manage connecting/reconnecting to Azure
// void connectToAzure()
// {
//   if (azureMqttClient.connected())
//     return;

//   Serial.print("Attempting Azure IoT Hub connection... ");

//   // Azure requires the ClientID, Username, and SAS Token Password
//   if (azureMqttClient.connect(client_id, mqtt_username, mqtt_password))
//   {
//     Serial.println("Connected to Azure!");
//     azureMqttClient.subscribe(c2d_topic);
//   }
//   else
//     Serial.printf("Failed connection, rc=%d. Try again in next loop.\n", azureMqttClient.state());
// }

// // The Azure telemetry topic format
// const char *d2c_topic = "devices/TestSensorHub/messages/events/";

// void sendAzureAlert(String alertMessage)
// {
//   if (!azureMqttClient.connected())
//     connectToAzure();

//   // Create a simple JSON payload
//   String payload = "{\"device\":\"TestSensorHub\",\"alert\":\"" + alertMessage + "\"}";

//   // Publish to Azure
//   if (azureMqttClient.publish(d2c_topic, payload.c_str()))
//     Serial.println("Alert successfully published to Azure!");
//   else
//     logger.add("Azure", "HUB", "Failed to publish alert to Azure");
// }

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
  WiFi.setTxPower(WIFI_POWER_13dBm);
  //? WiFi.persistent(false);
  WiFi.softAP("ESP_Hub", "SomeDumbPa$$22", 1, true); // hidden SSID
#if defined(BANOVO_BRDO)
  WiFi.begin(WIFI_SSID_VUJOVIC, WIFI_PASS_VUJOVIC);
#elif defined(VRANIC)
  WiFi.begin(WIFI_SSID_VRANIC, WIFI_PASS_VRANIC);
  // WiFi.begin(WIFI_SSID_MTS_UMKA, WIFI_PASS_MTS_UMKA);
  // WiFi.begin(WIFI_SSID_MTS_UMKA, WIFI_PASS_MTS_UMKA, 11); // this doesn't work
#endif
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println(" connected.");
  // Serial.print("ESP32 Web Server's IP address: ");
  // Serial.println(WiFi.localIP());
  // Serial.print("Channel: ");
  // Serial.println(WiFi.channel());

  // sntp_set_sync_interval(3 * 60 * SECOND); // TEST!!
  sntp_set_sync_interval(12 * 60 * 60 * SECOND); // sync every 12 hours
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
  Serial.print("Channel: ");
  Serial.println(WiFi.channel());

  // ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    while (true)
      delay(100);
  }
  // uint32_t version;
  // if (esp_now_get_version(&version) == ESP_OK)
  //   Serial.printf("ESP-NOW Protocol Version: v%s", version == 1 ? "1.0" : "2.0");
  // else
  //   Serial.println("Failed to fetch ESP-NOW version");
  setPeers();
  // esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // // Azure IoT Hub
  // azureSecureClient.setInsecure(); // Don't validate the TLS certificate chain (saves memory and avoids certificate expiration crashes)
  // azureMqttClient.setServer(mqtt_server, mqtt_port);
  // azureMqttClient.setCallback(azureCallback);
  actionNodes.init();
}

String message;

void loop()
{
  // ESP-NOW: handle Simple Event messages
  if (seh.isNewMessageReceived())
  {
    // Serial.println("main hub: seh.isNewMessageReceived()");
    auto peer = seh.getPeerInfo();
    if (peer != NULL)
    {
      // #ifdef BANOVO_BRDO
      if (peer->device == Device::ESP32BattConn)
      // #elif defined(VRANIC)
      // if (peer->device == Device::ESP32C3SuperMiniBlue)
      // #endif
      {
        auto notif = GetNotif(WaterDetected);
        if (notif != NULL)
        {
          if (notif->wa_msg)
          {
            // wifiConfig(false); //? test if this is necessary? probably not
            // delay(3000);
            // 💥Stan, kuhinja, sudopera:
            // VISOK NIVO VODE U SUDOPERI 💦
            auto res = NotifyWhatsApp::sendMessage("%F0%9F%92%A5+Stan,+kuhinja,+sudopera:%0AVISOK+NIVO+VODE+U+SUDOPERI!+%F0%9F%92%A6");
            if (res != CMB_OK)
              logger.add(CMB_LOG_TYPE, "ESP32Hub", NotifyWhatsApp::errorMessage(res));
            // wifiConfig(true);
          }
          if (notif->buzz)
            buzzer.blinkCritical();
        }
      }
#if defined(VRANIC)
      if (peer->device == Device::ESP32C3Xiao)
      {
        auto notif = GetNotif(MovementDetected);
        if (notif != NULL)
        {
          if (notif->wa_msg)
          {
            auto res = NotifyWhatsApp::sendMessage(seh.getMessageText());
            // auto res = NotifyWhatsApp::sendMessage("Movement+detected+in+the+room!+%F0%9F%94%B4");// Not tested
            // auto res = NotifyWhatsApp::sendMessage("Movement+detected+in+the+room"); // Tested OK
            if (res != CMB_OK)
              logger.add(CMB_LOG_TYPE, "ESP32Hub", NotifyWhatsApp::errorMessage(res));
          }
          if (notif->buzz)
            buzzer.blinkCritical();
        }
      }
#endif
      logger.add(ToString::SensorTypes[peer->type], ToString::Devices[peer->device], seh.getMessageText());
    }
    seh.clearEventData();
  }

  getLocalTime(&ti);
  tw.buzzIN();

  // restart ESP32 at 22:22:22 every day to get current time from NTP server
  // if (ti.tm_hour == 22 && ti.tm_min == 22 && ti.tm_sec == 22)
  //   ESP.restart();

  //* TEST if ESP can "Talk to the Internet (to sync NTP time, send data to Azure, etc.)"
  // if (ti.tm_hour == 23 && ti.tm_min == 58 && ti.tm_sec == 10)
  // {
  //   auto res = NotifyWhatsApp::sendMessage("Hey+now+:)");
  //   if (res != 200) // 200 = OK, log if not OK
  //     Serial.println("WhatsApp Bot error res: " + String(res));
  // }

  // if (!azureMqttClient.connected())
  // {
  //   static ulong lastReconnectAttempt = 0;
  //   ulong now = millis();
  //   if (now - lastReconnectAttempt > 10000) //* make this interval longer after a few tries
  //   {
  //     lastReconnectAttempt = now;
  //     connectToAzure();
  //   }
  // }
  // else
  //   azureMqttClient.loop();
  
  actionNodes.loop();

  delay(10);
}
