#pragma once

#include "Blinky.h" // https://github.com/bvujovic/ArduinoLibs/tree/main/Blinky

#define TW_BUZZONMIN_DEF (10)

struct BuzzData
{
    BuzzData(byte minutes, byte countBuzz, ulong itvBuzz)
        : minutes(minutes), countBuzz(countBuzz), itvBuzz(itvBuzz)
    {
    }
    byte minutes;
    byte countBuzz;
    ulong itvBuzz;
};

class TimeWatcher
{
private:
    BuzzData buzzes[6] = {
        BuzzData(0, 2, 1000),
        BuzzData(10, 1, 333),
        BuzzData(20, 2, 333),
        BuzzData(30, 1, 1000),
        BuzzData(40, 1, 333),
        BuzzData(50, 2, 333)};
    Blinky *buzzer = NULL;
    byte prevMinutesBuzzIN = 123;
    tm &ti;
    byte buzzOnMinOptions[4] = {5, 10, 30, 60};
    byte buzzOnMin = TW_BUZZONMIN_DEF;
    bool isItOn = false;

public:
    TimeWatcher(tm &ti) : ti(ti) {}
    void setBlinky(Blinky *b) { buzzer = b; }
    void buzzIN();
    void setIsItOn(bool on) { isItOn = on; }
    bool getIsItOn() { return isItOn; }
    void setBuzzOnMin(byte buzzOnMin)
    {
        setIsItOn(buzzOnMin > 0);
        for (byte o : buzzOnMinOptions)
            if (buzzOnMin == o)
                this->buzzOnMin = buzzOnMin;
    }
    byte getBuzzOnMin() { return buzzOnMin; }
    // B
    // void test()
    // {
    //     Serial.println(buzzOnMin = 10);
    //     for (BuzzData b : buzzes)
    //         if (b.minutes % buzzOnMin == 0)
    //             Serial.println(b.minutes);
    //     Serial.println(buzzOnMin = 30);
    //     for (BuzzData b : buzzes)
    //         if (b.minutes % buzzOnMin == 0)
    //             Serial.println(b.minutes);
    //     Serial.println(buzzOnMin = 60);
    //     for (BuzzData b : buzzes)
    //         if (b.minutes % buzzOnMin == 0)
    //             Serial.println(b.minutes);
    // }
};
