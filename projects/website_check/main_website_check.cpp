//* Device checks https://elektrodistribucija.rs/planirana-iskljucenja-beograd/...
//* to see if there are planned power outages.

#include <Arduino.h>
#include "WiFiServerBasics.h"
#include "PlannedOutagesChecker.h"
PlannedOutagesChecker poc;

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

    //* Testing POC funcionality when memory is low.
    // Serial.println(ESP.getFreeHeap() / 1024);
    // char *ptr = (char *)malloc(15 * 1024);
    // Serial.println(ptr != NULL);
    // Serial.println(ESP.getFreeHeap() / 1024);

    poc.loadLocations("/power_outage_locations.csv", true);
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
    Serial.flush();
#ifdef ESP32
    esp_sleep_enable_timer_wakeup(5 * 60 * 1000000); // wake up after 5 minutes
    esp_deep_sleep_start();
#elif defined(ESP8266)
    ESP.deepSleep(0); // sleep forever
#endif
}

void loop()
{
}
