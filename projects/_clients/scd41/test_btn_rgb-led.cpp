//* Testing button and RGB LED pin choices for ESP32-C3 Zero

#include <Arduino.h>

#include "OneButton.h" // lib_deps = mathertel/OneButton@^2.0.0
OneButton btn(9, true); //

#include <Adafruit_NeoPixel.h> // adafruit/Adafruit NeoPixel @ ^1.15.5
const byte pinLed = 10;
Adafruit_NeoPixel strip(1, pinLed, NEO_GRB + NEO_KHZ800);
void ledStart()
{
    strip.begin();
    strip.setBrightness(10);
    strip.clear();
    strip.show();
}
void ledOn(bool on)
{
    if (on)
        strip.setPixelColor(0, strip.Color(0, 255, 0));
    else
        strip.clear();
    strip.show();
}

void setup()
{
    Serial.begin(115200);
    ledStart();
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
