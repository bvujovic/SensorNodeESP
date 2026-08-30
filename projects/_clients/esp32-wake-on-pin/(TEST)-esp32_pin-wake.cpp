//* Testing if certain pin can wake XIAO ESP32-C3 up.

// #include <Arduino.h>
// void setup()
// {
//   Serial.begin(115200);
// }
// void loop()
// {
//   Serial.println(digitalRead(GPIO_NUM_3));
//   delay(500);
// }

#include <Arduino.h>
#include "esp_sleep.h"

//? D2 works (wakes XIAO up, but w/ delay of about 1-2sec); D0, D1 don't work
#define PIR_PIN D1      // connected to OUT of PIR (AM312)
const byte pinLed = D2; // test LED

void setup()
{
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, 1);
  Serial.begin(115200);
  delay(1000);
  digitalWrite(pinLed, 0);

  pinMode(PIR_PIN, INPUT);

  Serial.println("Going to sleep...");

  // Wake when GPIO3 becomes HIGH
  esp_deep_sleep_enable_gpio_wakeup(
      1ULL << PIR_PIN,
      ESP_GPIO_WAKEUP_GPIO_HIGH);

  delay(100);
  esp_deep_sleep_start();
}

void loop()
{
}
