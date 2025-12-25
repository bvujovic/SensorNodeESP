#pragma once

#include "Blinky.h" // https://github.com/bvujovic/ArduinoLibs/tree/main/Blinky

class MyBlinky
{
private:
    Blinky *blinky;

public:
    MyBlinky(uint8_t pin) { blinky = new Blinky(pin, true); }
    ~MyBlinky() { delete blinky; }
    Blinky *getBlinky() { return blinky; }

    void blinkOk() { blinky->blink(500, 2); }
    void blinkWarning() { blinky->blink(250, 3); }
    void blinkCritical() { blinky->ledOn("11.11.11.11.80", 333); }
};
