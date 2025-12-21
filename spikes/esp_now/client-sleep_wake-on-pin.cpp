/*
  Robust ESP32 deep sleep wake example
  - Wakes on a pin change (uses EXT0 / RTC GPIO)
  - Debounces the wake signal in software
  - If the wake is spurious, goes back to deep sleep immediately
  - If valid, blinks LED then goes back to sleep

  NOTES:
  - Use an RTC-capable GPIO for WAKE_PIN. Common RTC GPIOs on many ESP32 boards:
      0, 2, 4, 12, 13, 14, 15, 25, 26, 27
    (Check your exact module's datasheet if unsure.)
  - ext0/EXT0 (esp_sleep_enable_ext0_wakeup) supports one RTC pin and wakes on level.
*/

#include <Arduino.h>
#include "esp_sleep.h"

// const gpio_num_t WAKE_PIN = GPIO_NUM_4; // <- choose an RTC-capable pin
const gpio_num_t WAKE_PIN = GPIO_NUM_14; // <- choose an RTC-capable pin
const int LED_PIN = 22;                 // On-board LED (change if needed)
const int BLINK_COUNT = 3;
const int BLINK_MS = 180;
// Debounce / validation: sample this many times after wake
const int VALID_SAMPLES = 8;
const int SAMPLE_DELAY_MS = 8;

// Choose the active level that should indicate a real wake.
// For example, if you use INPUT_PULLUP and button to ground, ACTIVE_LEVEL should be 0 (LOW).
const int ACTIVE_LEVEL = 0; // 0 = LOW, 1 = HIGH

#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
const byte pinLed = 22; // LED_BUILTIN
void ledOn(bool on) { digitalWrite(pinLed, on); }
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
const byte pinLed = 2; // LED_BUILTIN
void ledOn(bool on) { digitalWrite(pinLed, !on); }
#endif

#include "MacAddresses.h"
uint8_t *mac = macSoftEsp32DevIpex;
bool sendSuccess = true;
ulong msStart = 0;

#ifdef ESP32
esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == ESP_NOW_SEND_SUCCESS) ? "Success" : "FAIL");
}
#else
void OnDataSent(uint8_t *mac, uint8_t sendStatus)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println((sendSuccess = sendStatus == 0) ? "Success" : "FAIL");
}
#endif

void blinkOnce()
{
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < BLINK_COUNT; ++i)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(BLINK_MS);
        digitalWrite(LED_PIN, LOW);
        delay(BLINK_MS);
    }
}

bool validateWakePin()
{
    // Sample the pin VALID_SAMPLES times with short delay and count matches to ACTIVE_LEVEL.
    int matches = 0;
    for (int i = 0; i < VALID_SAMPLES; ++i)
    {
        int v = digitalRead((int)WAKE_PIN);
        if (v == ACTIVE_LEVEL)
            matches++;
        delay(SAMPLE_DELAY_MS);
    }
    // Accept if a majority of samples show the ACTIVE_LEVEL
    return (matches * 2) >= VALID_SAMPLES;
}

void goToSleep()
{
    // Reconfigure pin to proper input with pull resistor to avoid floating while sleeping
    // if (ACTIVE_LEVEL == 0)
    //     pinMode((int)WAKE_PIN, INPUT_PULLUP);
    // else
    //     pinMode((int)WAKE_PIN, INPUT_PULLDOWN);
    pinMode((int)WAKE_PIN, ACTIVE_LEVEL == 0 ? INPUT_PULLUP : INPUT_PULLDOWN);

    // Use EXT0 to wake from a single RTC pin. level param: 0 => wake on LOW, 1 => wake on HIGH
    esp_sleep_enable_ext0_wakeup(WAKE_PIN, ACTIVE_LEVEL);

    Serial.println("Going to deep sleep now.");
    delay(100);
    esp_deep_sleep_start();
}

void setup()
{
    Serial.begin(115200);
    delay(10); // allow Serial to start

    esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
    Serial.printf("Wakeup reason: %d\n", (int)wakeReason);

    // Prepare the wake pin as input and internal pull (so it's not floating)
    // if (ACTIVE_LEVEL == 0)
    //     pinMode((int)WAKE_PIN, INPUT_PULLUP);
    // else
    //     pinMode((int)WAKE_PIN, INPUT_PULLDOWN);
    pinMode((int)WAKE_PIN, ACTIVE_LEVEL == 0 ? INPUT_PULLUP : INPUT_PULLDOWN);

    if (wakeReason == ESP_SLEEP_WAKEUP_EXT0)
    {
        Serial.println("Woke from EXT0 wakeup. Validating pin...");
        // Give the pin a moment to settle, then sample to filter noise
        delay(10);
        bool valid = validateWakePin();
        if (!valid)
            Serial.println("Wake appeared spurious/noisy -> going back to sleep.");
        else
        {
            Serial.println("Valid wake detected.");
            // blinkOnce();

            WiFi.mode(WIFI_STA);
            // Serial.println(WiFi.macAddress());
            if (esp_now_init() != ESP_OK)
            {
                Serial.println("ESP NOW INIT FAIL");
                goToSleep();
            }
#ifdef ESP32
            esp_now_register_send_cb(OnDataSent);
            memcpy(peerInfo.peer_addr, mac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            if (esp_now_add_peer(&peerInfo) != ESP_OK)
            {
                Serial.println("Failed to add peer");
                goToSleep();
            }
#else
            esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
            esp_now_register_send_cb(OnDataSent);
            esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
#endif
            //TODO change message, maybe send the same message 10 secondes after first one
            Serial.println("Sending initial message...");
            char str[] = "Hello ESP-Now";
            auto res = esp_now_send(mac, (uint8_t *)&str, strlen(str));
            Serial.printf("Send response: 0x%X\n", res);
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
