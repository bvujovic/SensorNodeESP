//* ATtiny sleeps, wakes up on interrupt, sends signal via HC-12, goes back to sleep
//* Sleep current: ~0.5mA

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <SoftwareSerial.h>

#define HC12_TX PB3  // ATtiny85 pin 2
#define HC12_RX PB4  // ATtiny85 pin 3
#define WAKE_PIN PB2 // ATtiny85 pin 7 (INT0)

SoftwareSerial HC12(HC12_TX, HC12_RX); // TX, RX

volatile bool wakeUpFlag = false;

ISR(INT0_vect)
{
    // Just set flag; don’t do Serial writes here
    wakeUpFlag = true;
}

void goToSleep()
{
    wakeUpFlag = false;

    // // Enable INT0 interrupt on LOW
    // GIMSK |= (1 << INT0);
    // MCUCR &= ~(1 << ISC01);
    // MCUCR &= ~(1 << ISC00);
    
    // Enable INT0 interrupt on Rising Edge
    GIMSK |= (1 << INT0);
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    
    // // Enable INT0 interrupt on 
    // GIMSK |= (1 << INT0);
    // MCUCR |= (1 << ISC00);
    // MCUCR &= ~(1 << ISC01);

    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sei();       // Enable global interrupts
    sleep_cpu(); // Go to sleep

    // CPU wakes here
    sleep_disable();
}

void setup()
{
    // pinMode(WAKE_PIN, INPUT_PULLUP); // External button/sensor pulls low
    pinMode(WAKE_PIN, INPUT);
    HC12.begin(4800);
}

void loop()
{
    goToSleep();

    if (wakeUpFlag)
    {
        HC12.println("Wake-up message from ATtiny85!");
        delay(1000); // give HC-12 some time
    }
}
