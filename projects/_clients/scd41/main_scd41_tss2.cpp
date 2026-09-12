//*
//* This version uses TimeSlotSend class to get time synchronization from the hub and to manage deep sleep intervals.

#include "Enums.h"
#include <SensirionI2CScd4x.h> // sensirion/Sensirion I2C SCD4x@^1.1.0
SensirionI2cScd4x scd;

#include "AirData.h"
AirData airData;
// #define MICROSECOND (1000000UL) // microseconds in a second
#define SECOND (1000UL)      // milliseconds in a second
#define MINUTE (60 * SECOND) // milliseconds in a minute

const byte maxRetries = 3; // Max retries for getting data from sensors
byte cntRetries = 0;       // Counter for retries
bool shouldRetry = false;  // Flag to indicate if we should retry reading sensors

const byte pinLed = 10;
void ledOn(bool on) { digitalWrite(pinLed, on); }
void ledOnDelay(int secs)
{
  ledOn(true);
  delay(secs * SECOND);
  ledOn(false);
}

#include "MacAddresses.h"
#include <esp_now.h>
#include <WiFi.h>
#if defined(BANOVO_BRDO)
uint8_t *mac = macSoftEsp32DevIpex;
#elif defined(VRANIC)
uint8_t *mac = macEsp32BattConnVranic;
#endif
uint8_t macFail[] = {0x78, 0x1C, 0x3C, 0xCA, 0xF3, 0x33}; // Non-existent MAC for testing
esp_now_peer_info_t peerInfo;

#include "TimeSlotSend.h"
TimeSlotSend tss(5, 5, 20, 6, 60);
#include "ClientLogger.h"
ClientLogger logger;
// #include "OneButton.h"   // lib_deps = mathertel/OneButton@^2.0.0
// OneButton btn(D7, true); // click: send data, 2click: print log, long click: clear log
// bool sendDataNow = false;

void sendTimeRequest()
{
  // Serial.println("Sending time request...");
  auto res = esp_now_send(mac, (uint8_t *)tss.getCmdTime(), strlen(tss.getCmdTime()));
  if (res != 0)
    logger.add("ESP-NOW time request send error: " + String(res));
  tss.timeReqIsSent(millis());
}

// void OnDataSent(uint8_t *mac, uint8_t sendStatus)
// void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
// {
//   if (sendStatus != 0)
//     // Serial.println("Last Packet Send Status: FAIL!!!");
//     logger.add("ESP-NOW last packet send FAILED, status: " + String(sendStatus), tss.getStrTime());
// }

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  // Serial.println("Time response received.");
  tss.onTimeStringRecv(incomingData, len, millis(), true);
}

void goToSleep(uint64_t usSleepTime)
{
  Serial.println("GO TO SLEEP");
  delay(100); // wait for send callback
  ESP.deepSleep(usSleepTime);
  // esp_sleep_enable_timer_wakeup( );
  // esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  pinMode(pinLed, OUTPUT);
  ledOn(true);
  delay(2000); //* Wait for testing purposes
  ledOn(false);
  Serial.println("Starting ESP32 SCD41 Sensor Client with Time Slot Send");
  tss.setItvTimeRespWait(5000); // 5 sec to wait for time response
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_13dBm); // adjust power for wifi antenna, default is max power
  while (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW INIT FAIL");
    logger.add("ESP-NOW init error", tss.getStrTime());
    ledOnDelay(10);
    goToSleep(tss.getDeepSleepTime());
  }
  // auto res = esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  auto res = esp_now_register_recv_cb(OnDataRecv);
  if (res != ESP_OK)
  {
    Serial.printf("Register receiver code: %X\n", res);
    logger.add("ESP-NOW register recv cb error: " + String(res), tss.getStrTime());
    // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    // esp_now_register_send_cb(OnDataSent);
    ledOnDelay(10);
    goToSleep(tss.getDeepSleepTime());
  }

  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0; // use current channel
  peerInfo.encrypt = false;
  res = esp_now_add_peer(&peerInfo);
  if (res != ESP_OK)
  {
    printf("esp_now_add_peer res: 0x%X\n", res);
    // logger.add("ESP-NOW add peer error: " + String(res), tss.getStrTime());
    ledOnDelay(10);
    goToSleep(tss.getDeepSleepTime());
  }
  else
    sendTimeRequest();

  Wire.begin(8, 9);
  scd.begin(Wire, 0x62); // Use the default I2C address for SCD41 (0x62)

  Serial.println("Setup complete, waiting for time response...");
}

void loop()
{
  if (tss.isTimeToSendData(millis()))
  {
    // printf("Sending data millis: %lu\n", millis());
    // Wire.begin();          // Adjust SDA/SCL pins here if needed (e.g., Wire.begin(21, 22))
    // Wire.begin(8, 9);
    // scd.begin(Wire, 0x62); // Use the default I2C address for SCD41 (0x62)
    scd.stopPeriodicMeasurement();
    auto error = scd.measureSingleShot();
    if (error)
    {
      Serial.println("Failed to trigger single shot.");
      goToSleep(tss.getDeepSleepTime());
    }
    // Wait for measurement completion
    bool isDataReady = false;
    ulong startTime = millis();
    const ulong TIMEOUT_MS = 6000;

    // Poll until data is ready or timeout expires
    while (!isDataReady && (millis() < startTime + TIMEOUT_MS))
    {
      delay(100); // yield to avoid hammering the I2C bus
      scd.getDataReadyStatus(isDataReady);
    }
    Serial.printf("Single-Shot Measurement Time: %lu ms\n", millis() - startTime);

    if (isDataReady)
    {
      uint16_t co2 = 0;
      float temp = 0.0f, humidity = 0.0f;
      error = scd.readMeasurement(co2, temp, humidity);
      if (!error)
      {
        Serial.printf("CO2: %d ppm | T: %.2f °C | RH: %.2f %%\n", co2, temp, humidity);
        airData.ECO2 = co2;
        airData.temperature = temp;
        airData.humidity = humidity;
        airData.status = 0; // Normal operation
        // esp now send
        auto res = esp_now_send(mac, (uint8_t *)&airData, sizeof(airData));
        Serial.printf("ESP-NOW send result: 0x%X\n", res);
      }
      delay(1000);
    }
    else
    {
      Serial.println("Data not ready yet.");
      ledOnDelay(10);
      // goToSleep(tss.getDeepSleepTime());
      goToSleep((tss.getSlotMin() * 60 - 30) * 1000000UL);
    }
    delay(100); // wait for send callback
    goToSleep(tss.getDeepSleepTime());
  }

  // repeat sendTimeRequest() if the answer (time) is not received for more than 1 sec
  if (tss.isTimeRespMissing(millis()))
  {
    Serial.println("Time response missing, sending time request again...");
    sendTimeRequest();
  }
  if (tss.getIsWakeUpTimeWrong())
  {
    // logger.add("Wake-up time is off! Diff: " + String(tss.getItvWrongTimeDiff()) + " seconds", tss.getStrTime());
    // for (size_t i = 0; i <= 20; i++)
    // {
    //   ledOn(i % 2);
    //   delay(100);
    // }
    Serial.println("Wake-up time is off! Diff: " + String(tss.getItvWrongTimeDiff()) + " seconds");
    ledOnDelay(10);
    tss.resetWakeUpTimeWrong();
  }
  delay(20);
}
