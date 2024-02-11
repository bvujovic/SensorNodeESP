#include <Arduino.h>
#include <WiFiServerBasics.h>
ESP8266WebServer server(80);
#include <ESP8266HTTPClient.h>
#include <CredCallMeBot.h>

const byte pinLed = LED_BUILTIN;
const byte pinWater = D3;

void ledOn(bool turnLedOn)
{
  digitalWrite(pinLed, !turnLedOn);
}

#define DEBUG
#ifdef DEBUG
#define writeln(x) Serial.println(x)
#define serialDebugBegin() Serial.begin(115200)
#else
#define writeln(x)
#define serialDebugBegin()
#endif

void wiFiOn()
{
  writeln("wiFiOn");
  ledOn(true);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  while (!ConnectToWiFi())
  {
    writeln("Connecting to WiFi failed. Device will try to connect again...");
    for (ulong i = 0; i < 10; i++)
    {
      ledOn(i % 2);
      delay(500);
    }
    delay(1 * 60 * 1000); // wait for 1 minute
  }
  writeln("WiFi ON");
  ledOn(false);
}

void setup()
{
  serialDebugBegin();
  pinMode(pinLed, OUTPUT);
  pinMode(pinWater, INPUT_PULLUP);
  wiFiOn();
}

void loop()
{
  delay(200);
  if (!digitalRead(pinWater))
  {
    //* https://www.callmebot.com/blog/free-api-whatsapp-messages/
    String url = "http://api.callmebot.com/whatsapp.php?";
    url = url + "phone=" + CMB_PHONE;
    // 💥Stan, kuhinja, sudopera:
    // VISOK NIVO VODE U SUDOPERI 💦
    url = url + "&text=" + "%F0%9F%92%A5+Stan,+kuhinja,+sudopera:%0AVISOK+NIVO+VODE+U+SUDOPERI+%F0%9F%92%A6";
    url = url + "&apikey=" + CMB_API_KEY;
    writeln(url);

    WiFiClient wiFiClient;
    HTTPClient client;
    client.begin(wiFiClient, url);
    int respCode = client.GET();
    writeln("Resp code: ");
    writeln(respCode);
    if (respCode > 0)
      writeln(client.getString());
      // TODO poruka se salje max 3x na po 5min
    else
      ; // TODO pokusavanje 3x na po 
    client.end();
  }
  // TODO mozda napraviti da se ovo ispitivanje izvrsava 1 u minutu ili tako nesto, a ne ovako cesto
  if (!WiFi.isConnected())
    ESP.reset();
}
