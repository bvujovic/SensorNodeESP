//* Battery (Li-ion) powered ESP8266 sleeps, wakes up and checks if LDR detects low level of light.
//* If that's true (some LED/light is switched off) - buzzer will beep couple of times.

#include <Arduino.h>

const byte pinIn = A0; // LDR
const byte pinOut = 4; // D2, buzzer

const uint64_t shortSleep = 5;       // (seconds) Duration of sleep when LED/light is still on.
const uint64_t longSleep = 3;        // (minutes) Duration of sleep after LED/light is switched off.
const uint64_t usToSec = 1000000;    // Million microseconds = 1 second.
const uint32_t cntBeeps = 2;         // How many times will buzzer go off when LED/light is switched off.
const int lightLevelThreshold = 256; // If detected light is below this level - buzzer go off.

void setup()
{
    pinMode(pinIn, INPUT);
    int a = analogRead(pinIn);
    if (a < lightLevelThreshold)
    {
        pinMode(pinOut, OUTPUT);
        for (uint32_t i = 0; i < cntBeeps; i++)
        {
            digitalWrite(pinOut, 1);
            delay(1000);
            digitalWrite(pinOut, 0);
            delay(1000);
        }
        ESP.deepSleep(longSleep * 60 * usToSec, RF_DISABLED);
    }
    else
        ESP.deepSleep(shortSleep * usToSec, RF_DISABLED);
}

void loop()
{
}
