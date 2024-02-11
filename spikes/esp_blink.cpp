#include <Arduino.h>
#include <Blinky.h>

void setup()
{
    Blinky led = Blinky::create(500, 4);
    led.blink();
}

void loop()
{
}
