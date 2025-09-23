//* ATtiny sleeps, wakes up on interrupt, sends signal via HC-12, goes back to sleep
//* This code is executing on ATtiny85 @1MHz w/ HC-12 (transmitter).
//* Sleep current: ~0.1mA


#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include <SoftwareSerial.h>
#define PIN_RX 3 // HC-12 RX Pin
#define PIN_TX 4 // HC-12 TX Pin
SoftwareSerial HC12(PIN_RX, PIN_TX);
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

ISR(INT0_vect)
{
    cli();
    cmd = CMD_SIGNAL;
}

void setup()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    ADCSRA &= ~bit(ADEN); // Turn off ADC to save power.

    // pinMode(PB2, SIGNAL_ON == HIGH ? INPUT : INPUT_PULLUP);
#if SIGNAL_ON == HIGH
//* NOT TESTED YET!
// For PIR sensor, we need to wait a bit after setting pin to INPUT
//    pinMode(PB2, INPUT);
//    for (int i = 0; i < 5 * 1000; i++)
//        delayMicroseconds(1000);
//    // TODO zameniti ovo fiksno cekanje od 5sec za cekanje dok se signal ne spusti na LOW, npr...
//    while (bit_is_set(PINB, PB2))
//        for (int i = 0; i < 200; i++)
//            delayMicroseconds(1000);
#else
    pinMode(PB2, INPUT_PULLUP);
#endif

//    PCMSK |= _BV(PCINT1); // Configure pin change interrupt.
//    GIFR |= bit(PCIF);    // clear any outstanding interrupts
//    GIMSK |= bit(PCIE);   // enable pin change interrupts
    GIMSK |= (1 << INT0);
    MCUCR &= ~(1 << ISC01);
    MCUCR &= ~(1 << ISC00);
}

int i = 123;
char buff[20];

void send()
{
    i++;
//    HC12.write("Tst\n");
//    HC12.write(String(i).c_str());
    sprintf(buff, " Test:%03d.\n", i % 1000);
    HC12.write(buff);
}

void loop()
{
    //?pinMode(PIN_TX, INPUT); // Set PIN_TX to INPUT in order to save power.
    sei();                  // Enable interrupts again, go to sleep and wait for intterrupt.
    go_to_sleep();

    cli(); // disable interupts
    if (cmd != CMD_NONE)
    {
        // Wakes up here.
        //?pinMode(PIN_TX, OUTPUT);
        HC12.begin(4800);
        delay(10); // Wait a bit for HC-12 to stabilize.

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
