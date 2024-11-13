#include <Arduino.h>
#include <Blinky.h>

void setup()
{
    Blinky led = Blinky::create(500, 4);
    // Blinky led = Blinky(22, false);
    led.blink();
}

void loop()
{
}
