//* Sketch tests if the HTTPClient can send request after WiFi.config() call. Solution: reset ESP :\
//* ESP reads "setIP" bool from Preferences, if it's true - it sets static IP using WiFi.config(),
//* setIP is flipped and saved to Preferences. sendWhatsAppMessage() is called: success if WiFi.config() isn't called.
//* build_flags = -DCORE_DEBUG_LEVEL=5

//* https://github.com/espressif/arduino-esp32/issues/493
//* https://github.com/espressif/arduino-esp32/issues/2778
//* https://stackoverflow.com/questions/68508224/getting-ewifigeneric-cpp739-hostbyname-dns-failed-when-performing-post-r


#include <WiFi.h>
#include <HTTPClient.h>
#include <CredWiFi.h>
#include <CredCallMeBot.h>

#include <Preferences.h>
Preferences preferences;

// ulong msStart;
IPAddress ipa;
IPAddress gateway;
IPAddress subnet(255, 255, 255, 0);

int sendWhatsAppMessage()
{
    // String url = "http://api.callmebot.com/whatsapp.php?";
    String url = "/whatsapp.php?";
    url = url + "phone=" + CMB_PHONE;
    url = url + "&text=" + "Testsss!";
    url = url + "&apikey=" + CMB_API_KEY;
    Serial.println(url);
    WiFiClient wiFiClient;
    HTTPClient client;
    // client.begin(wiFiClient, url);
    // client.begin(url);
    // client.begin("api.callmebot.com", 80, url);
    client.begin(wiFiClient, "13.38.186.117", 80, url);

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

    preferences.begin("wa");
    bool setIP = preferences.getBool("setIP", false);
    Serial.printf("\nSetIP: %d\n", setIP);
    preferences.putBool("setIP", !setIP);
    preferences.end();

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

    // if (setIP)
    {
        WiFi.config(IPAddress(192, 168, 0, 80), IPAddress(192, 168, 0, 254), subnet);
        Serial.print("ESP32 Web Server's IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.gatewayIP());
        Serial.println(WiFi.subnetMask());
    }
    sendWhatsAppMessage();
    ESP.restart();
}

void loop()
{
    delay(100);
}
