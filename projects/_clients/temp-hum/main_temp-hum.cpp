#include <Arduino.h>

const byte pinSendOften = 4; // GPIO4 (D4) - LOW = send every minute, HIGH = send every 10 minutes
const byte pinDHT = 3;       // DHT sensor pin on ESP32
#include <DHT.h>             // lib_deps = adafruit/DHT sensor library@^1.4.6
#define DHTTYPE DHT22        // DHT 22 (AM2302)
DHT dht(pinDHT, DHTTYPE);

#include "AirData.h"
AirData airData;

const byte maxRetries = 3; // Max retries for getting data from sensors
byte cntRetries = 0;       // Counter for retries
bool shouldRetry = false;  // Flag to indicate if we should retry reading sensors
bool sendDataNow = false;

#include "MacAddresses.h"
#include <esp_now.h>
#include <WiFi.h>
uint8_t *mac = macEsp32BattConnVranic;
// uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing

#include "TimeSlotSend.h"
TimeSlotSend tss(2, 5, 15, 1, 30);

void sendTimeRequest()
{
  auto res = esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
  if (res != 0)
    Serial.println("ESP-NOW time request send error: " + String(res));
  tss.timeReqIsSent(millis());
}

esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
  Serial.print("Last Packet Send Status: ");
  Serial.println((sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  tss.onTimeStringRecv(incomingData, len, millis(), true);
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

  pinMode(pinSendOften, INPUT_PULLUP);
  tss.setSlotMin((digitalRead(pinSendOften) == HIGH ? 10 : 1));

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_13dBm);
  while (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW INIT FAIL");
    Serial.println(tss.getStrTime());
    delay(5000);
  }
  auto res = esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  Serial.println("ESP-NOW register send cb error: " + String(res));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    //? goToSleep();
  }
  res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("ESP-NOW register recv cb error: " + String(res));

  sendTimeRequest();
}

void loop()
// { // TEST: print values from DHT sensor
//   float humidity = dht.readHumidity();
//   float temperature = dht.readTemperature();
//   Serial.print("Humidity: ");
//   Serial.print(humidity);
//   Serial.print("%  Temperature: ");
//   Serial.print(temperature);
//   Serial.println("°C");
//   delay(2000);
// }
{
  if (tss.isTimeToSendData(millis()) || sendDataNow)
  {
    cntRetries = 0;
    do
    {
      // printf("Reading sensors millis: %lu\n", millis());
      shouldRetry = false;
      float hum = dht.readHumidity();
      float temp = dht.readTemperature();
      if (isnan(hum) || isnan(temp))
      {
        Serial.println("Failed to read from DHT sensor!");
        airData.temperature = 0;
        airData.humidity = 0;
        shouldRetry = true;
      }
      if (hum > 100)
        shouldRetry = true;
      airData.temperature = temp;
      airData.humidity = (int)(hum + 0.5); // Round to nearest integer
      Serial.print("Temp: ");
      Serial.print(airData.temperature);
      Serial.print(", Hum: ");
      Serial.print(airData.humidity);
      Serial.println("% rH");

      if (shouldRetry)
      {
        cntRetries++;
        // Serial.printf("Retrying... (%d/%d)\n", cntRetries, maxRetries);
        Serial.println("Retrying sensor read... (" + String(cntRetries) + "/" + String(maxRetries) + ")");
        // , tss.getStrTime()
        delay(2000);
      }
    } while (shouldRetry && (cntRetries < maxRetries));

    // printf("Sending data millis: %lu\n", millis());
    if (cntRetries < maxRetries)
    {
      Serial.println("Sending data via ESP-NOW");
      auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
      if (res != ESP_OK)
      {
        // printf("Send res: 0x%X\n", res);
        Serial.println("ESP-NOW send error: " + String(res));
        //, tss.getStrTime()
        delay(5000);
      }
    }
    // if data is sent on click - do not go to sleep and wait for time slot to send data
    if (sendDataNow)
      sendDataNow = false;
    else
    {
      delay(100); // wait for send callback
      Serial.println("GO TO SLEEP");
      ESP.deepSleep(tss.getDeepSleepTime());
    }
  }
  // repeat sendTimeRequest() if the answer (time) is not received for more than 1 sec
  if (tss.isTimeRespMissing(millis()))
    sendTimeRequest();
  if (tss.getIsWakeUpTimeWrong())
  {
    Serial.println("Wake-up time is off! Diff: " + String(tss.getItvWrongTimeDiff()) + " seconds");
    // if ESP woke up just a bit too late - send data anyway
    if (tss.getItvWrongTimeDiff() > 540) // more than 9 minutes = less than 1 minute late
      sendDataNow = true;
    else
      Serial.println("Oh too late, too late.");
    tss.resetWakeUpTimeWrong();
  }
  delay(20);
}
