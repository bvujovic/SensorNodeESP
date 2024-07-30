//* ATtiny sleeps, wakes up on HIGH (PIR...), sends signal via STX882, goes back to sleep
//* This code is executing on ATtiny85 @8MHz w/ STX882 (transmitter).
//* Based on https://github.com/perja12/nexa_remote_control

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

const byte pinTx = 0; // DATA pin on STX882
//* const byte pinIn = 2; // PIR, wires for water detection, buton... (INPUT_PULLUP)

#define CMD_NONE 0
#define CMD_MIDDLE 2

volatile int cmd = CMD_NONE;

// TODO isprobati ovo sa PIRom i zicama za vodu umesto tastera
// TODO da li moze bez cmd-a? mozda samo bool promenljiva da se vidi da li se desio interrupt
// TODO u konacnoj verziji bi se n puta poslalo po m signala od x ms - izvuci konstante na vr' koda
// TODO dodati sliku test elektronike i README.md fajl sa objasnjenjima

ISR(PCINT0_vect)
{
    cli();
    if (!bit_is_set(PINB, PB2))
        cmd = CMD_MIDDLE;
}

void setup()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Turn off ADC to save power.
    ADCSRA &= ~bit(ADEN);

    pinMode(PB2, INPUT_PULLUP);

    // Configure pin change interrupt.
    PCMSK |= _BV(PCINT2);
    GIFR |= bit(PCIF);  // clear any outstanding interrupts
    GIMSK |= bit(PCIE); // enable pin change interrupts
}

// Transmit HIGH/LOW and then delay for itv milliseconds.
void send(uint8_t val, uint8_t itv)
{
    digitalWrite(pinTx, val);
    delayMicroseconds(itv * 1000);
}

void loop()
{
    pinMode(pinTx, INPUT); // Set pinTx to INPUT in order to save power.
    sei();                 // Enable interrupts again, go to sleep and wait for intterrupt.
    go_to_sleep();

    cli(); // disable interupts
    if (cmd != CMD_NONE)
    {
        // Wakes up here.
        pinMode(pinTx, OUTPUT);

        for (int j = 0; j < 3; j++)
        {
            // BV: 10x send LOW for 5000 microsec
            for (int i = 0; i < 10; i++)
            {
                send(HIGH, 1);
                send(LOW, 5);
            }
            send(HIGH, 1);
            send(LOW, 1);

            for (int i = 0; i < 1000; i++)
                delay(50);
        }
    }
    // Avoid getting a new interrupt because of button bounce.
    delay(50);
    sei(); // enable interupts

    cmd = CMD_NONE;
}

void go_to_sleep()
{
    power_all_disable(); // power off ADC, Timer 0 and 1, serial interface
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    power_all_enable(); // power everything back on
}
