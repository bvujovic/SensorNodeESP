//* ATtiny sleeps, wakes up on LOW (test button, water detection), blinks 3x, goes back to sleep
//* This code is executing on ATtiny85 @1MHz.

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define PIN_TX 0 // DATA pin on STX882
//* #define PIN_IN 1; // PB1 - wire for water detection, button (INPUT_PULLUP); PIR (INPUT)

#define CNT_REPEAT_SEND 3    // How many times signal is sent.
#define ITV_PAUSE 2          // (seconds) Pause between sending signals.
#define ITV_PULSE 5000       // (microseconds) Duration of 1 data pulse (LOW).
#define ITV_BREAK_PULSE 1000 // (microseconds) Duration of 1 break pulse (HIGH).
#define ITV_PULSE_SEND 10    // How many pulses are sent for 1 signal.

#define CMD_NONE 0
#define CMD_SIGNAL 1

#define SIGNAL_ON LOW // This value should be HIGH for PIR, LOW for water detection wires, test button...

volatile uint8_t cmd = CMD_NONE;

ISR(PCINT0_vect)
{
    cli();
    if (bit_is_set(PINB, PB1) == (SIGNAL_ON == HIGH))
        cmd = CMD_SIGNAL;
}

void setup()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    ADCSRA &= ~bit(ADEN); // Turn off ADC to save power.

    // pinMode(PB1, SIGNAL_ON == HIGH ? INPUT : INPUT_PULLUP);
#if SIGNAL_ON == HIGH
    pinMode(PB1, INPUT);
    for (int i = 0; i < 5 * 1000; i++)
        delayMicroseconds(1000);
    // TODO zameniti ovo fiksno cekanje od 5sec za cekanje dok se signal ne spusti na LOW, npr...
    while (bit_is_set(PINB, PB1))
        for (int i = 0; i < 200; i++)
            delayMicroseconds(1000);
#else
    pinMode(PB1, INPUT_PULLUP);
#endif

    // Configure pin change interrupt.
    PCMSK |= _BV(PCINT1);
    GIFR |= bit(PCIF);  // clear any outstanding interrupts
    GIMSK |= bit(PCIE); // enable pin change interrupts
}

// Transmit HIGH/LOW and then delay for itv microseconds.
void send()
{
    digitalWrite(PIN_TX, 1);
    delay(250);
    digitalWrite(PIN_TX, 0);
}

void loop()
{
    pinMode(PIN_TX, INPUT); // Set PIN_TX to INPUT in order to save power.
    sei();                  // Enable interrupts again, go to sleep and wait for intterrupt.
    go_to_sleep();

    cli(); // disable interupts
    if (cmd != CMD_NONE)
    {
        // Wakes up here.
        pinMode(PIN_TX, OUTPUT);

        uint8_t j = 0;
        while (true)
        {
            send();

            if (++j >= CNT_REPEAT_SEND)
                break;

            for (int i = 0; i < ITV_PAUSE * 1000; i++)
                delayMicroseconds(1000);
        }
    }
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
