#include <Arduino.h>

const byte pinSet = 8;

void setup()
{
    pinMode(pinSet, OUTPUT);
    digitalWrite(pinSet, HIGH); // For data transmission
    // digitalWrite(pinSet, LOW); // For AT commands, e.g. AT+RX
    Serial2.begin(9600); // TX2, RX2: UART connection to HC-12
    // Serial1.begin(9600); // TX1, RX1: UART connection to HC-12
    Serial.begin(115200);
    Serial.println("\n *** HC12 Start ***");
}

void loop()
{
    while (Serial2.available())
    {
        Serial.println(Serial2.readString());
        // Serial.write(Serial2.read()); // Send the data to Serial monitor
    }
    //   while (Serial.available())
    //   {                            // If Serial monitor has data
    //     HC12.write(Serial.read()); // Send that data to HC-12
    //   }
}
