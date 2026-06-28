//* Same as esp32-wake-on-pin.cpp but with sleep between send attempts
/*
Supported boards:
CONFIG_IDF_TARGET_ESP32
CONFIG_IDF_TARGET_ESP32C3
//CONFIG_IDF_TARGET_ESP32S3
*/

#include <Arduino.h>
#include "esp_sleep.h"

#define MSG_TEXT "Water detected!"
#define MAX_SEND_ATTEMPTS 3
#define SEC_REPEAT_SEND_DELAY 4 // Interval in seconds between send attempts
#define MIN_COOL_DOWN 20        // Device will not respond to pin events for this many minutes
#define ACTIVE_LEVEL HIGH       // Level that indicate a wake: LOW/HIGH

#if defined(CONFIG_IDF_TARGET_ESP32)
const gpio_num_t pinWake = GPIO_NUM_14;
const byte pinLed = 22; // On-board LED
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
const gpio_num_t pinWake = GPIO_NUM_4;
const byte pinLed = 8; // On-board LED
// #elif defined(CONFIG_IDF_TARGET_ESP32S3)
// const gpio_num_t pinWake = GPIO_NUM_14; // example
#else
#error Unsupported target
#endif

void ledOn(bool on) { digitalWrite(pinLed, !on); }

#include <esp_now.h>
#include <WiFi.h>
#include "Enums.h"
#include "MacAddresses.h"
#if defined(BANOVO_BRDO)
uint8_t *mac = macSoftEsp32DevIpex;
#elif defined(VRANIC)
uint8_t *mac = macEsp32BattConnVranic;
#endif
bool sendSuccess = true;
ulong msStart = 0;
RTC_DATA_ATTR int cntSendAttempt = 0;
RTC_DATA_ATTR uint32_t msgId;

esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
  Serial.print("Last Packet Send Status: ");
  Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}

bool validateWakePin()
{
  // Debounce / validation: sample this many times after wake
  const int cntSamples = 8;
  const int itvSampleDelay = 8;
  // Sample the pin cntSamples times with short delay and count matches to ACTIVE_LEVEL.
  int matches = 0;
  for (int i = 0; i < cntSamples; ++i)
  {
    int v = digitalRead((int)pinWake);
    if (v == ACTIVE_LEVEL)
      matches++;
    delay(itvSampleDelay);
  }
  // Accept if a majority of samples show the ACTIVE_LEVEL
  return (matches * 2) >= cntSamples;
}

// Go to sleep and wake up on pin event
void goToSleepWakeOnPin()
{
#if CONFIG_IDF_TARGET_ESP32C3
  // Enable GPIO wakeup
  gpio_wakeup_enable(pinWake, ACTIVE_LEVEL == LOW ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  // Enable wakeup source
  esp_deep_sleep_enable_gpio_wakeup(1 << pinWake, ACTIVE_LEVEL == LOW ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH);
#else
  esp_sleep_enable_ext0_wakeup(pinWake, ACTIVE_LEVEL); // Use EXT0 to wake from a single RTC pin. level param: 0 => wake on LOW, 1 => wake on HIGH
#endif
  Serial.println("Going to deep sleep now.");
  delay(100);
  esp_deep_sleep_start();
}

// Go to sleep and wake up after secs seconds
void goToSleepForSeconds(int secs)
{
  delay(100);
  esp_sleep_enable_timer_wakeup(secs * 1000000ULL);
  esp_deep_sleep_start();
}

void goToSleep()
{
  if (cntSendAttempt >= MAX_SEND_ATTEMPTS)
  {
    cntSendAttempt = 0;
    printf("Cool down period - device will not respond to pin events for %.1f minutes.\n", (float)MIN_COOL_DOWN);
    goToSleepForSeconds(MIN_COOL_DOWN * 60);
  }
  else
    goToSleepForSeconds(SEC_REPEAT_SEND_DELAY);
}

void sendEspNowMessage()
{
  cntSendAttempt++;
  WiFi.mode(WIFI_STA);
#if CONFIG_IDF_TARGET_ESP32C3
  WiFi.setTxPower(WIFI_POWER_13dBm); // adjust power for wifi antenna, default is max power
#endif
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW INIT FAIL");
    goToSleep();
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    goToSleep();
  }
  char message[80];
  sprintf(message, "%lu;%s", msgId, MSG_TEXT);
  printf("Sending message: '%s'...\n", message);
  auto res = esp_now_send(mac, (uint8_t *)message, strlen(message));
  if (res != ESP_OK)
    Serial.printf("Send response: 0x%X\n", res);
  goToSleep();
}

void setup()
{
  Serial.begin(115200);
  delay(10); // allow Serial to start
  pinMode(pinLed, OUTPUT);
  ledOn(false);

  auto wakeReason = esp_sleep_get_wakeup_cause();
  Serial.printf("Wakeup reason: %d\n", (int)wakeReason);
  // Prepare the wake pin as input and internal pull (so it's not floating)
  pinMode((int)pinWake, ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
#if CONFIG_IDF_TARGET_ESP32C3
  if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
#else
  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0)
#endif
  {
    Serial.println("Woke up from GPIO. Validating pin...");
    delay(10); // Give the pin a moment to settle, then sample to filter noise
    auto valid = validateWakePin();
    if (!valid)
    {
      Serial.println("Wake appeared spurious/noisy -> going back to sleep.");
      goToSleepWakeOnPin();
    }
    else
    {
      Serial.println("Valid wake detected.");
      msgId = esp_random();
      sendEspNowMessage();
    }
  }
  else
  {
    if (cntSendAttempt > 0)
      sendEspNowMessage();
    else
    {
      Serial.println("Normal boot or other wake. Going to sleep and waiting for wake event.");
      goToSleepWakeOnPin();
    }
  }
}

void loop() {}
