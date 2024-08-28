//* Device checks https://elektrodistribucija.rs/planirana-iskljucenja-beograd/...
//* to see if there are planned power outages.
//TODO Refactor this code, make a class, add sending WhatsApp msg and deploy it in some other project.

#include <Arduino.h>
#include "WiFiServerBasics.h"
#include <WiFiClient.h>

const String webFiles[] = {
    "Dan_0_Iskljucenja.htm",
    "Dan_1_Iskljucenja.htm",
    "Dan_2_Iskljucenja.htm",
};
const String WEB_HOST = "elektrodistribucija.rs";

struct Target
{
    String municipality;
    String street;
};

const Target targets[] = {
    {"Барајево", "ПОП-БОРИНА"},
    {"Чукарица", "КРАЉИЦЕ КАТАРИНЕ"},
    {"Чукарица", "МИЛИЈЕ СТАНОЈЛОВИЋА"},
};

void setup()
{
    Serial.begin(115200);
    Serial.println();
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    while (!ConnectToWiFi())
    {
        Serial.println("Connecting to WiFi failed.");
        return;
    }

    // Serial.println(ESP.getFreeHeap() / 1024); 48

    for (auto webFile : webFiles)
    {
        WiFiClientSecure client;
        client.setInsecure();
        if (!client.connect(WEB_HOST, 443))
        {
            Serial.println("Connection to host failed!");
            return;
        }

        Serial.println("----");
        Serial.println(webFile);
        if (client.connected())
        {
            client.print(String("GET /planirana-iskljucenja-beograd/") + webFile + " HTTP/1.1\r\n" +
                         "Host: " + WEB_HOST + "\r\n" +
                         "Connection: close\r\n\r\n");
            delay(10);
        }
        ulong timeout = millis();
        while (client.available() == 0)
            if (millis() - timeout > 5000)
            {
                Serial.println("Client Timeout!");
                client.stop();
                return;
            }
        // Serial.println(ESP.getFreeHeap() / 1024); 20
        String line;
        int cntTargets = sizeof(targets) / sizeof(Target);
        while (client.available())
        {
            line = client.readStringUntil('\n');
            if (line.indexOf("<HTML>") == -1)
                continue;

            for (int i = 0; i < cntTargets; i++)
            {
                int lastIdxEnd = 0;
                while (lastIdxEnd >= 0)
                {
                    int idxStart = line.indexOf(targets[i].municipality, lastIdxEnd);
                    if (idxStart == -1)
                    {
                        lastIdxEnd = -1;
                        continue;
                    }
                    int idxEnd = lastIdxEnd = line.indexOf("</TR>", idxStart);
                    if (idxEnd == -1)
                        continue;
                    int idxStreet = line.indexOf(targets[i].street, idxStart);
                    if (idxStreet == -1 || idxStreet > idxEnd)
                        continue;
                    Serial.println(targets[i].municipality);
                    Serial.println(targets[i].street);
                }
            }
        }
        Serial.println(ESP.getFreeHeap() / 1024); // 11
        line.clear();
        client.stop();
    }
    Serial.println("Closing connection...");
    // Serial.println(ESP.getFreeHeap() / 1024); 31

    ESP.deepSleep(0);
}

void loop()
{
    delay(100);
}
