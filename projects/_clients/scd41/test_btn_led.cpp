//* Testing button and LED pin choices

#include <Arduino.h>

#include "OneButton.h"  // lib_deps = mathertel/OneButton@^2.0.0
OneButton btn(7, true); // click: send data, 2click: print log, long click: clear log

const byte pinLed = 10;
void ledOn(bool on) { digitalWrite(pinLed, !on); }

void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("test begin");
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    delay(500);
    ledOn(true);
    delay(1500);
    ledOn(false);
    delay(500);
    Serial.println("test end");

    btn.attachClick([]()
                    {
        Serial.println("click");
        ledOn(true);
        delay(1000);
        ledOn(false); });

    btn.attachLongPressStart([]()
                             { 
        Serial.println("long click");
        ledOn(false);
        delay(300);
        ledOn(true); });
}

void loop()
{
    btn.tick();
    delay(20);
}
