//* Device checks https://elektrodistribucija.rs/planirana-iskljucenja-beograd/...
//* to see if there are planned power outages.
// TODO Refactor this code, make a class, add sending WhatsApp msg and deploy it in some other project.

#include <Arduino.h>
#include "WiFiServerBasics.h"
#include <WiFiClient.h>

const String webFiles[] = {
    "Dan_0_Iskljucenja.htm",
    "Dan_1_Iskljucenja.htm",
    "Dan_2_Iskljucenja.htm",
};
const String WEB_HOST = "elektrodistribucija.rs";

struct Location
{
    String municipality;
    String street;
    bool found;
};

Location locations[] = {
    {"Чукарица", "КРАЉИЦЕ КАТАРИНЕ", false},
    {"Чукарица", "МИЛИЈЕ СТАНОЈЛОВИЋА", false},
    {"Барајево", "ПОП-БОРИНА", false},
};

void searchForLocations(String &html)
{
    for (auto &l : locations)
    {
        l.found = false;
        int lastIdxEnd = 0;
        while (lastIdxEnd >= 0)
        {
            int idxStart = html.indexOf(l.municipality, lastIdxEnd);
            if (idxStart == -1)
            {
                lastIdxEnd = -1;
                continue;
            }
            int idxEnd = lastIdxEnd = html.indexOf("</TR>", idxStart);
            if (idxEnd == -1)
                continue;
            int idxStreet = html.indexOf(l.street, idxStart);
            if (idxStreet == -1 || idxStreet > idxEnd)
                continue;
            l.found = true;
        }
    }
}

void printFoundLocations()
{
    for (auto &l : locations)
        if (l.found)
            Serial.println(l.municipality + ": " + l.street);
}

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

    WiFiClientSecure *client = new WiFiClientSecure();
    client->setInsecure();
    if (!client->connect(WEB_HOST, 443))
    {
        Serial.println("Connection to host failed!");
        return;
    }

    for (auto &webFile : webFiles)
    {
        Serial.println(webFile);
        // if (client->connected())
        // {
        client->print(String("GET /planirana-iskljucenja-beograd/") + webFile + " HTTP/1.1\r\n" +
                      "Host: " + WEB_HOST + "\r\n" +
                      "Connection: Keep-Alive\r\n\r\n");
        delay(10);
        // }
        ulong timeout = millis();
        while (client->available() == 0)
            if (millis() - timeout > 5000)
            {
                Serial.println("Client Timeout!");
                client->stop();
                return;
            }
        String line;
        while (client->available())
        {
            line = client->readStringUntil('\n');
            if (line.indexOf("<HTML>") == -1)
                continue;
            searchForLocations(line);
            printFoundLocations();
        }
        // Serial.println(ESP.getFreeHeap() / 1024); // 11, 9, 17
    }
    client->stop();
    delete client;

    Serial.println("Going to sleep...");
    ESP.deepSleep(0);
}

void loop()
{
    delay(100);
}
