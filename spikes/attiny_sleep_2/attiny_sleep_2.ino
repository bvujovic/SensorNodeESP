//* https://www.avrfreaks.net/s/topic/a5C3l000000UbEiEAK/t156928

#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

volatile uint8_t LED = 0;

int main(void)
{
    GIMSK = 0b01000000; // interupt as INT0 defined
    DDRB = 0b00000001;  // set PB0 as output
    PORTB = 0b00000110; // set Pull-Up-Resistor at PB1 and PB2

    while (1)
    {
        MCUCR = 0b00110000;         // sleep enabled (SE=1), Power Down mode (SM1=1, SM0=0)
        sei();                      // enable interrupts
        sleep_mode();               // start sleeping and wait for INT0
        cli();                      // disable interrupts
        MCUCR &= ~(1 << SE);        // sleep disable
        PORTB = PORTB | (1 << PB0); // light on
        LED = 1;                    // marker that LED is on now
        sei();
        while (LED == 1)
            ; // waiting until next button-press (ISR) will change to LED=0
        cli();
        PORTB = PORTB & ~(1 << PB0); // light off and restart the loop
    }
}

ISR(INT0_vect)
{
    _delay_ms(50);                // debouncing time
    if ((PINB & (1 << PB2)) == 0) // check if PB2 still grounded
    {
        LED = 0; // escape from while-loop by changing status to zero
    }
}
