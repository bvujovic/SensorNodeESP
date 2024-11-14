#include <Arduino.h>
const byte pinLed = 22;
const byte pinSet = 0;

char str[5];

void setup()
{
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, true);
    pinMode(pinSet, OUTPUT);
    digitalWrite(pinSet, HIGH);
    Serial2.begin(9600); // Serial port to HC12
}

int cnt = 0;

void loop()
{
    delay(2000);
    digitalWrite(pinLed, false);
    Serial2.write(itoa(cnt++, str, 10));
    digitalWrite(pinLed, true);
}