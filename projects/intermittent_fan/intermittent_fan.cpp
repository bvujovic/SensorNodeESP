//* Fan works for 2min, then it's off for some time (2, 4...).

#include <Arduino.h>

const byte pinBin1 = D1;
const byte pinBin2 = D2;
const byte pinPWMB = D5;

#include <Btn.h> // Button class from project MeinClock
Btn btn(D6);

int idx = -1;                         // Index for intervals array. -1 -> fan works non-stop
byte intervals[] = {2, 4, 8, 16, 32}; // (minutes) Different options for pause intervals.
byte cntIntervals;                    // Number of elements in intervals array.
byte itvWorking = 2;                  // (minutes) How much time will fan work.
ulong msStarted;                      // (milliseconds) When was the fan last started.
bool isWorking;

const ulong SEC = 1000;
const ulong MIN = 60 * SEC;

// start or stop the fan
void fanWorks(bool on)
{
    // Serial.printf("fanWorks: %d, %d, %8ld\n", on, idx, millis() / 1000);
    // analogWrite(pinPWMB, 106); // 5/12 * 255
    digitalWrite(pinBin1, on);
    digitalWrite(LED_BUILTIN, !on);
    isWorking = on;
    if (on)
    {
        msStarted = millis();
        delay(1000);
        digitalWrite(pinBin1, 0);
        digitalWrite(pinBin2, 1);
        delay(1000);
        digitalWrite(pinBin1, 1);
        digitalWrite(pinBin2, 0);
    }
}

void setup()
{
    // Serial.begin(115200);
    // Serial.println("---------");

    pinMode(pinBin1, OUTPUT);
    pinMode(pinBin2, OUTPUT);
    pinMode(pinPWMB, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    analogWriteResolution(8);
    analogWriteFreq(18000);
    digitalWrite(pinBin2, 0);
    digitalWrite(pinPWMB, HIGH);

    fanWorks(true);
    cntIntervals = sizeof(intervals) / sizeof(byte);
}

void loop()
{
    delay(10);

    if (idx != -1)
    {
        if (millis() > msStarted + (itvWorking + intervals[idx]) * MIN && !isWorking)
            fanWorks(true);
        if (millis() > msStarted + itvWorking * MIN && isWorking)
            fanWorks(false);
    }

    // btn.Update();
    ClickType click = btn.check(millis());
    if (click == ShortClick) // use next interval in intervals array
    {
        if (idx < cntIntervals - 1)
            idx++;
        fanWorks(true);
    }
    if (click == LongClick) // reset index to -1 - fan works without breaks
    {
        idx = -1;
        fanWorks(true);
    }
}
