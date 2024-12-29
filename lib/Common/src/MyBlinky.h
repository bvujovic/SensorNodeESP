#pragma once

#include "Blinky.h"

class MyBlinky
{
private:
    Blinky *blinky;

public:
    MyBlinky(uint8_t pin) { blinky = new Blinky(pin, true); }
    ~MyBlinky() { delete blinky; }

    void blinkOk() { blinky->blinkOk(); }
    void blinkWarning() { blinky->blinkWarning(); }
    void blinkError() { blinky->patternedBlink("11.11.11.11.80", 333); }
};

// B
// class MyBlinky : public Blinky
// {
// public:
//     MyBlinky(int pin, bool onValue, ulong msec = 500, ulong count = 3)
//         : Blinky(pin, onValue, msec, count) {}
//     // void blinkWarning() { blink(250, 3); }
//     void blinkError() { patternedBlink("11.11.11.11.80", 333); }
// };
