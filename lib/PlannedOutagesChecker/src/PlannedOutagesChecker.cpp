#include "PlannedOutagesChecker.h"
#include <WiFiClientSecure.h>

PocResult PlannedOutagesChecker::check(String &msg)
{
    if (locations.size() == 0)
    {
        msg = "No locations are specified for search.";
        return PocError;
    }
#ifdef ESP32
    const int minMemoryKB = 50;
#elif defined(ESP8266)
    const int minMemoryKB = 28;
#endif

    if (ESP.getFreeHeap() < minMemoryKB * 1024)
    {
        msg = "Memory is too low for creating WiFiClientSecure.";
        return PocError;
    }
    const char* WEB_HOST = "elektrodistribucija.rs";
    const String webFiles[] = {
        "Dan_0_Iskljucenja.htm",
        "Dan_1_Iskljucenja.htm",
        "Dan_2_Iskljucenja.htm",
    };
    // Serial.println(ESP.getFreeHeap() / 1024);
    WiFiClientSecure *client = new WiFiClientSecure();
    client->setInsecure();
    if (!client->connect(WEB_HOST, 443))
    {
        msg = "Connection to host failed!";
        return PocError;
    }
    // Serial.println(ESP.getFreeHeap() / 1024);
    for (auto &webFile : webFiles)
    {
        Serial.println(webFile);
        client->print(String("GET /planirana-iskljucenja-beograd/") + webFile + " HTTP/1.1\r\n" +
                      "Host: " + WEB_HOST + "\r\n" +
                      "Connection: Keep-Alive\r\n\r\n");
        delay(10);
        ulong timeout = millis();
        while (client->available() == 0)
            if (millis() - timeout > 5000)
            {
                client->stop();
                msg = "Web Client Timeout!";
                return PocError;
            }
        String line;
        while (client->available())
        {
            line = client->readStringUntil('\n');
            if (line.indexOf("Content-Length") != -1)
            {
                // e.g. Content-Length: 1234
                int idx = line.indexOf(":");
                if (idx != -1)
                {
                    String s = line.substring(idx + 1);
                    uint contentLength = s.toInt();
                    if (ESP.getFreeHeap() < contentLength + 1024)
                    {
                        msg = "Memory is too low for reading page: " + webFile;
                        return PocError;
                    }
                }
            }
            if (line.indexOf("<HTML>") == -1)
                continue;
            searchForLocations(line);
            foundLocationsToString(webFile, msg);
        }
        // Serial.println(ESP.getFreeHeap() / 1024);
    }
    client->stop();
    delete client;
    return msg.length() == 0 ? PocNoOutages : PocOutagesFound;
}

void PlannedOutagesChecker::searchForLocations(String &html)
{
    // for (auto &l : locations)
    for (int i = 0; i < locations.size(); i++)
    {
        Location *l = locations.get(i);
        l->found = false;
        int lastIdxEnd = 0;
        while (lastIdxEnd >= 0)
        {
            int idxStart = html.indexOf(l->municipality, lastIdxEnd);
            if (idxStart == -1)
            {
                lastIdxEnd = -1;
                continue;
            }
            int idxEnd = lastIdxEnd = html.indexOf("</TR>", idxStart);
            if (idxEnd == -1)
                continue;
            int idxStreet = html.indexOf(l->street, idxStart);
            if (idxStreet == -1 || idxStreet > idxEnd)
                continue;
            l->found = true;
        }
    }
}

void PlannedOutagesChecker::foundLocationsToString(const String &webFile, String &msg)
{
    // for (auto &l : locations)
    for (int i = 0; i < locations.size(); i++)
    {
        Location *l = locations.get(i);
        if (l->found)
            msg += webFile + ": " + l->municipality + ", " + l->street + "\n";
    }
}

void PlannedOutagesChecker::addLocation(const String &municipality, const String &street)
{
    locations.add(new Location{municipality, street, false});
}

bool PlannedOutagesChecker::loadLocations(const String &fileName, bool openCloseFS)
{
    LittleFS.begin();
    File fp = LittleFS.open(fileName, "r");
    if (!fp)
        return false;
    String mun, street;
    while (fp)
    {
        mun = fp.readStringUntil('|');
        if (mun.length() == 0)
            break;
        street = fp.readStringUntil('\n');
        addLocation(mun, street);
    }
    fp.close();
    LittleFS.end();
    return true;
}
