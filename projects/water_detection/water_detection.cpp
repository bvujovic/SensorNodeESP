//* Device (ESP8266) is always connected to WiFi and as soon as it gets the signal (water detected)
//* it sends WhatsApp message. If message fail to send, device will try again several times.

#include <Arduino.h>
#include <WiFiServerBasics.h>
ESP8266WebServer server(80);
#include <ESP8266HTTPClient.h>
#include <CredCallMeBot.h>

#include "Blinky.h" // https://github.com/bvujovic/ArduinoLibs/tree/main/Blinky
Blinky led = Blinky::create();

// To print or not to print - debug messages. Also, DEBUG -> fewer retries for msg send failures.
#define DEBUG true
#if DEBUG
#define writeln(x) Serial.println(x)
#define serialDebugBegin() Serial.begin(115200)
#else
#define writeln(x)
#define serialDebugBegin()
#endif

// Sending an actual WhatsApp message or not (testing purposes).
#define REAL_SEND_MSG

// Milliseconds in 1 second.
#define SEC (1000)
// Milliseconds in 1 minute.
#define MIN (60 * SEC)

const byte pinWater = D3;

bool isMsgSent = false;
// Moment when the last message was sent (msec).
ulong msLastMsgSent;
// Device can't send message again in less than {noResendInterval} milliseconds.
const ulong noResendInterval = (DEBUG ? 1 : 15) * MIN;
// Retry intervals (in seconds) for failed message sends.
#if DEBUG
uint retryIntervals[] = {0, 5, 20};
#else
uint retryIntervals[] = {0, 5, 30, 3 * 60, 15 * 60};
#endif
// Index/counter for retries (message send failures). -1 when there is no problem with sending messages.
int idxRetry = -1;

void wiFiOn()
{
#ifdef REAL_SEND_MSG
  writeln("wiFiOn");
  led.on();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  while (!ConnectToWiFi())
  {
    writeln("Connecting to WiFi failed. Device will try to connect again...");
    led.blink(5, 500);
    led.on();
    delay(1 * MIN);
  }
  led.off();
  writeln("Connected to WiFi");
#endif
}

int sendWhatsAppMessage()
{
  writeln("sendWhatsAppMessage");
#ifdef REAL_SEND_MSG
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
  client.end();
  return respCode;
#else // fake send message
  led.blinkOk();
  return HTTP_CODE_BAD_REQUEST;
  // return HTTP_CODE_OK;
#endif
}

void setup()
{
  serialDebugBegin();
  pinMode(led.getPin(), OUTPUT);
  pinMode(pinWater, INPUT_PULLUP);
  wiFiOn();
}

void loop()
{
  delay(200);
  ulong itv = millis() - msLastMsgSent;
  // Is it OK to send (another) message: 1st message or enough time has passed since the last one.
  bool lastSuccSendClear = !isMsgSent || itv > noResendInterval;
  // Should device try to send message again after failed send.
  bool lastFailSendClear = idxRetry > -1 && itv > retryIntervals[idxRetry] * SEC;

  if ((!digitalRead(pinWater) && lastSuccSendClear) || lastFailSendClear)
  {
    msLastMsgSent = millis();
    int respCode = sendWhatsAppMessage();

    if (respCode == HTTP_CODE_OK)
      isMsgSent = true;
    else
    {
      // Incrementing retry index but if the index reached the end of {retryIntervals} - stop (re)trying.
      if ((uint)++idxRetry >= sizeof(retryIntervals) / sizeof(uint))
      {
        isMsgSent = true;
        idxRetry = -1;
      }
      writeln(idxRetry);
    }
  }
#ifndef FAKE_SEND_MSG
  if (!WiFi.isConnected())
  {
    writeln("No WiFi -> reset device and try to connect to WiFi again.");
    ESP.reset();
  }
#endif
}
