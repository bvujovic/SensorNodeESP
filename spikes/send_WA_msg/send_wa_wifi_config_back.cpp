//* Sketch tests if the HTTPClient can send request after WiFi.config() call.
//* Solution: WiFi.config(IPAddress(0, 0, 0, 0)... and than wait 3sec or so

#include <WiFi.h>
#include <HTTPClient.h>
#include <CredWiFi.h>
#include <CredCallMeBot.h>

ulong msStart;
IPAddress ipa;
IPAddress gateway;
IPAddress subnet(255, 255, 255, 0);

int sendWhatsAppMessage()
{
    // String url = "http://kingtrader.info/php/index.html"; // test URL
    String url = "http://api.callmebot.com/whatsapp.php?";
    // String url = "/whatsapp.php?";
    url = url + "phone=" + CMB_PHONE;
    url = url + "&text=" + "Testsss!";
    url = url + "&apikey=" + CMB_API_KEY;
    Serial.println(url);
    WiFiClient wiFiClient;
    HTTPClient client;
    client.begin(wiFiClient, url);
    // client.begin(url);
    // client.begin("api.callmebot.com", 80, url);
    // client.begin(wiFiClient, "13.38.186.117", 80, url);

    int respCode = client.GET();
    Serial.println("Resp code: ");
    Serial.println(respCode);
    if (respCode > 0)
        Serial.println(client.getString());
    client.end();
    return respCode;
}

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_AP_STA);
    //? WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("\nConnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(500);
    }
    Serial.println(" connected.");
    Serial.println(ipa = WiFi.localIP());
    Serial.println(gateway = WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
    // sendWhatsAppMessage(); // OK
    WiFi.config(IPAddress(192, 168, 0, 80), IPAddress(192, 168, 0, 254), subnet);
    delay(5000);
    // sendWhatsAppMessage(); // fail
    // WiFi.config(ipa, gateway, subnet); // original settings - before WiFi.config() call
    WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
    delay(3000); // no delay - fail, 2500ms - fail, 4000 - succ, 3000 - succ
    sendWhatsAppMessage(); // OK
    msStart = millis();
    while (true)
        delay(100);
}

bool wifiConfigSetBack = false;

void loop()
{
    if (!wifiConfigSetBack && millis() > msStart + 5000)
    {
        sendWhatsAppMessage(); // fail
        Serial.println("WiFi config back!");
        WiFi.config(ipa, gateway, subnet);
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.gatewayIP());
        Serial.println(WiFi.subnetMask());
        sendWhatsAppMessage(); // OK
        // WiFi.reconnect();
        // WiFi.disconnect(true, true);
        // WiFi.begin(WIFI_SSID, WIFI_PASS);
        // while (WiFi.status() != WL_CONNECTED)
        // {
        //     Serial.print('.');
        //     delay(1000);
        // }
        // Serial.println(" connected.");
        // Serial.println(WiFi.localIP());
        // Serial.println(WiFi.gatewayIP());
        // Serial.println(WiFi.subnetMask());
        wifiConfigSetBack = true;
    }

    if (millis() > msStart + 10000)
    {
        sendWhatsAppMessage(); // OK
        msStart = LONG_MAX;
    }
    delay(100);
}
