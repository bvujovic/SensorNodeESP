#include <Arduino.h>
#include <Blinky.h>

void setup() {
  Blinky led = Blinky::create(2000, 2);
  led.blink();
  ESP.deepSleep(0);
}

void loop() {
}
