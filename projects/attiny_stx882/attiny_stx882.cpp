//* ATtiny sleeps, wakes up on HIGH (PIR...), sends signal via STX882, goes back to sleep
//* This code is executing on ATtiny85 @8MHz w/ STX882 (transmitter).
//* Based on https://github.com/perja12/nexa_remote_control

//* Power consumption
//* Deep sleep: 14.6uA - PIR, 0.2uA - water detection wires
//* 0.16mA water detection: when wires are in the water (after signal is sent)
//* 13mA sending signals, 5mA waiting between sending signals

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define PIN_TX 0 // DATA pin on STX882
//* #define PIN_IN 1; // wire for water detection, button (INPUT_PULLUP); PIR (INPUT)

#define CNT_REPEAT_SEND 3    // How many times signal is sent.
#define ITV_PAUSE 4          // (seconds) Pause between sending signals.
#define ITV_PULSE 5000       // (microseconds) Duration of 1 data pulse (LOW).
#define ITV_BREAK_PULSE 1000 // (microseconds) Duration of 1 break pulse (HIGH).
#define ITV_PULSE_SEND 10    // How many pulses are sent for 1 signal.

#define CMD_NONE 0
#define CMD_SIGNAL 1

#define SIGNAL_ON HIGH // This value should be HIGH for PIR, LOW for water detection wires, test button...

volatile uint8_t cmd = CMD_NONE;

// TODO+ u konacnoj verziji bi se n puta poslalo po m signala od x ms - izvuci konstante na vr' koda
// TODO+ uslovno prevodjenje za PIR (signalizacija na HIGH) ili za taster/vodu (sign za LOW)
// TODO dodati sliku test elektronike i README.md fajl sa objasnjenjima

ISR(PCINT0_vect)
{
    cli();

    // if (bit_is_set(PINB, PB1) == (SIGNAL_ON == HIGH))
    //     cmd = CMD_SIGNAL;
#if SIGNAL_ON == HIGH
    if (bit_is_set(PINB, PB1))
#else
    if (!bit_is_set(PINB, PB1))
#endif
        cmd = CMD_SIGNAL;
}

void setup()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Turn off ADC to save power.
    ADCSRA &= ~bit(ADEN);

    // pinMode(PB1, SIGNAL_ON == HIGH ? INPUT : INPUT_PULLUP);
#if SIGNAL_ON == HIGH
    pinMode(PB1, INPUT);
#else
    pinMode(PB1, INPUT_PULLUP);
#endif

    for (int i = 0; i < 5 * 1000; i++)
        delayMicroseconds(1000);

    // Configure pin change interrupt.
    PCMSK |= _BV(PCINT1);
    GIFR |= bit(PCIF);  // clear any outstanding interrupts
    GIMSK |= bit(PCIE); // enable pin change interrupts
}

// Transmit HIGH/LOW and then delay for itv microseconds.
void send(uint8_t val, uint16_t itv)
{
    digitalWrite(PIN_TX, val);
    delayMicroseconds(itv);
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
            for (int i = 0; i < ITV_PULSE_SEND; i++)
            {
                send(HIGH, ITV_BREAK_PULSE);
                send(LOW, ITV_PULSE);
            }
            // send(HIGH, ITV_BREAK_PULSE);
            // send(LOW, ITV_BREAK_PULSE);

            if (++j >= CNT_REPEAT_SEND)
                break;

            for (int i = 0; i < ITV_PAUSE * 1000; i++)
                delayMicroseconds(1000);
        }
    }
    // delay(50); //Avoid getting a new interrupt because of button bounce.
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
