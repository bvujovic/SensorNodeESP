#include <Arduino.h>
#include <Blinky.h>

void setup() {
  Blinky led = Blinky::create(333, 3);
  led.blink();
  // https://www.espressif.com/sites/default/files/9b-esp8266-low_power_solutions_en_0.pdf
  // https://stackoverflow.com/questions/39481196/how-to-wake-esp8266-from-deep-sleep-without-continuous-resets
  // https://www.esp8266.com/viewtopic.php?f=8&t=390
  
  ESP.deepSleep(0);
}

void loop() {
}
