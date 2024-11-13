//* Test code for the server side. ESP8266 writes duration of pulses from SRX882.
//* These values can be analyzed in Excel...

#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include <WiFi.h>

const byte pinIn = 23; // DATA on SRX882

void wiFiOff()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    // WiFi.forceSleepBegin();
}

void setup()
{
    wiFiOff();
    pinMode(pinIn, INPUT);
    Serial.begin(115200);
}

void loop()
{
    ulong pulse = pulseIn(pinIn, LOW);
    if (pulse >= 3500 && pulse < 5500)
        Serial.println(pulse);
}
