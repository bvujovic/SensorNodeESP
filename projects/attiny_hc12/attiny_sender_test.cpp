//* Attiny85 HC-12 Sender Example.
//* ATtiny doesn't sleep, just sends signal via HC-12 in a loop.
//* It works with 1Mhz clock.

#include <SoftwareSerial.h>
const byte pinRx = 3; // HC-12 RX Pin
const byte pinTx = 4; // HC-12 TX Pin
SoftwareSerial HC12(pinRx, pinTx);
const byte pinLed = 0;

void setup()
{
    pinMode(pinLed, OUTPUT);
    HC12.begin(4800);
}

int i = 0;
char buff[20];

void loop()
{
    digitalWrite(pinLed, 1);
    i++;
    //  HC12.write(String(i).c_str());
    sprintf(buff, " Testing %03d.\n", i % 1000);
    HC12.write(buff);
    digitalWrite(pinLed, 0);
    delay(i % 5 == 0 ? 5000 : 2000);
}
