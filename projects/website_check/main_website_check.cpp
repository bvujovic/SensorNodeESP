//* Device checks https://elektrodistribucija.rs/planirana-iskljucenja-beograd/...
//* to see if there are planned power outages.
// TODO Add sending WhatsApp msg and deploy it in some other project.

#include <Arduino.h>
#include "WiFiServerBasics.h"
#include "PlannedOutagesChecker.h"

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

    // Serial.println(ESP.getFreeHeap() / 1024);

    PlannedOutagesChecker poc;
    poc.addLocation("Чукарица", "КРАЉИЦЕ КАТАРИНЕ");    // ja
    poc.addLocation("Чукарица", "МИЛИЈЕ СТАНОЈЛОВИЋА"); // poso
    poc.addLocation("Чукарица", "МОМЧИЛА ЧЕДИЋА");      // porodica
    poc.addLocation("Чукарица", "КАДИЊАЧ");             // Zec
    poc.addLocation("Барајево", "ПОП-БОРИНА");          // selo
    poc.addLocation("Барајево", "ШЕСТЕ ЛИЧКЕ ДИВИЗИЈЕ");
    String msg;
    PocResult res = poc.check(msg);
    switch (res)
    {
    case PocError:
        Serial.println(String("Error: ") + msg);
        break;
    case PocNoOutages:
        Serial.println("No locations are found.");
        break;
    case PocOutagesFound:
        Serial.print(String("Power outages on specified locations!\n") + msg);
        break;
    }
    Serial.println("Going to sleep...");
    ESP.deepSleep(0);
}

void loop()
{
}
