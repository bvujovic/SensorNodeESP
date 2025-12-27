// NodeMCU ESP8266 pulse transmitter for STX882
// Sends 10 pulses: LOW (5 ms), HIGH (1 ms)

#include <Arduino.h>

const int TX_PIN = D2;          // Choose any suitable digital pin
const int PULSE_LOW_US = 4950;  // 5 ms = 5000 µs
const int PULSE_HIGH_US = 1000; // 1 ms = 1000 µs
const int PULSE_COUNT = 4;

void sendPulses()
{
    // HIGH break
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(PULSE_HIGH_US);

    for (int i = 0; i < PULSE_COUNT; i++)
    {
        // Transmit LOW pulse
        digitalWrite(TX_PIN, LOW);
        delayMicroseconds(PULSE_LOW_US);

        // HIGH break
        digitalWrite(TX_PIN, HIGH);
        delayMicroseconds(PULSE_HIGH_US);
    }
}

void setup()
{
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH); // idle HIGH if needed

    delay(1000); // small startup delay
    sendPulses();
    delay(4000);
    sendPulses();
}

void loop()
{
    delay(1000);
}
