//* ESP32 device that wakes on a pin event and sends an ESP-NOW message
//* to a predefined MAC address and then goes back to deep sleep.
/*
    ESP32 Deep Sleep Wake on Pin
  NOTES:
  - Use an RTC-capable GPIO for WAKE_PIN. Common RTC GPIOs on many ESP32 boards:
      0, 2, 4, 12, 13, 14, 15, 25, 26, 27 (Check your exact module's datasheet if unsure.)
  - ext0/EXT0 (esp_sleep_enable_ext0_wakeup) supports one RTC pin and wakes on level.
*/

#include <Arduino.h>
#include "esp_sleep.h"

#define MSG_TEXT ("Water detected!")
const int maxRetries = 2;
const int itvRepeatSendDelay = 4000;
const gpio_num_t pinWake = GPIO_NUM_14; // <- choose an RTC-capable pin
const byte pinLed = 22;                 // On-board LED (change if needed)

// Choose the active level that should indicate a real wake.
// For example, if you use INPUT_PULLUP and button to ground, ACTIVE_LEVEL should be 0 (LOW).
const int activeLevel = 0; // 0 = LOW, 1 = HIGH

#include <esp_now.h>
#include <WiFi.h>
const byte pinLed = 22; // LED_BUILTIN
void ledOn(bool on) { digitalWrite(pinLed, on); }

#include "MacAddresses.h"
uint8_t *mac = macSoftEsp32DevIpex;
bool sendSuccess = true;
ulong msStart = 0;

esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}

void blinks()
{
    pinMode(pinLed, OUTPUT);
    for (int i = 0; i < 3; ++i)
    {
        digitalWrite(pinLed, HIGH);
        delay(200);
        digitalWrite(pinLed, LOW);
        delay(200);
    }
}

bool validateWakePin()
{
    // Debounce / validation: sample this many times after wake
    const int cntSamples = 8;
    const int itvSampleDelay = 8;
    // Sample the pin cntSamples times with short delay and count matches to activeLevel.
    int matches = 0;
    for (int i = 0; i < cntSamples; ++i)
    {
        int v = digitalRead((int)pinWake);
        if (v == activeLevel)
            matches++;
        delay(itvSampleDelay);
    }
    // Accept if a majority of samples show the activeLevel
    return (matches * 2) >= cntSamples;
}

void goToSleep()
{
    // Reconfigure pin to proper input with pull resistor to avoid floating while sleeping
    pinMode((int)pinWake, activeLevel == 0 ? INPUT_PULLUP : INPUT_PULLDOWN);
    // Use EXT0 to wake from a single RTC pin. level param: 0 => wake on LOW, 1 => wake on HIGH
    esp_sleep_enable_ext0_wakeup(pinWake, activeLevel);
    Serial.println("Going to deep sleep now.");
    delay(100);
    esp_deep_sleep_start();
}

void sendMessage(char *str)
{
    auto res = esp_now_send(mac, (uint8_t *)&str, strlen(str));
    if (res != ESP_OK)
        Serial.printf("Send response: 0x%X\n", res);
}

void setup()
{
    Serial.begin(115200);
    delay(10); // allow Serial to start

    esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
    Serial.printf("Wakeup reason: %d\n", (int)wakeReason);
    // Prepare the wake pin as input and internal pull (so it's not floating)
    pinMode((int)pinWake, activeLevel == 0 ? INPUT_PULLUP : INPUT_PULLDOWN);
    if (wakeReason == ESP_SLEEP_WAKEUP_EXT0)
    {
        Serial.println("Woke from EXT0 wakeup. Validating pin...");
        delay(10); // Give the pin a moment to settle, then sample to filter noise
        auto valid = validateWakePin();
        if (!valid)
            Serial.println("Wake appeared spurious/noisy -> going back to sleep.");
        else
        {
            Serial.println("Valid wake detected.");
            // blinkOnce();
            WiFi.mode(WIFI_STA);
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
            auto msgId = esp_random();
            char message[80];
            sprintf(message, "%lu;%s", msgId, MSG_TEXT);
            printf("Sending message: '%s'", message);
            // char str[] = "Hello ESP-Now";
            auto cntRetry = 1;
            while (true)
            {
                sendMessage(message);
                if (cntRetry++ >= maxRetries)
                    break;
                delay(itvRepeatSendDelay);
            }
            // TODO maybe sleep shouldn't be forever - it should last x minutes
            Serial.println("Going to deep sleep forever...");
            delay(100);
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
            esp_deep_sleep_start();
        }
    }
    else
        Serial.println("Normal boot or other wake. Going to sleep and waiting for wake event.");
    goToSleep();
}

void loop() {}
