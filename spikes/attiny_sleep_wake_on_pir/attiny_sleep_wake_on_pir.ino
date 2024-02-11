/*
  Opis: pistanje kad zec predje na zabranjeni deo kauca, kod nekih zica i sl.
  Arduino: ATtiny85
  Napajanje: 18650
  Senzori:
    - PIR: detekcija zivotinje
    - 2 tastera: vol+ i vol- za buzzer
  Aktuatori:
    - Buzzer
*/
//* ~500uA

#include <avr/sleep.h>
#include <avr/interrupt.h>

const int pinPIR = 2;           // PIR senzor; ako se promeni ova vrednost, obavezno izmeniti PCINT? u sleep() i setup()
const int pinBuzz = 0;          // buzzer/zvucnik

void setup()
{
  pinMode(pinPIR, INPUT);
  pinMode(pinBuzz, OUTPUT);
  digitalWrite(pinBuzz, false);

  GIMSK |= _BV(PCIE);   // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT2); // Use PBX as interrupt pin
  sei();                // Enable interrupts
}

void sleep()
{
  GIMSK |= _BV(PCIE);                  // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT2);                // Use PBX as interrupt pin
  ADCSRA &= ~_BV(ADEN);                // ADC off
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // replaces above statement
  sleep_enable();                      // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                               // Enable interrupts
  sleep_cpu();                         // sleep
}

ISR(PCINT0_vect)
{
}

void loop()
{
  if (digitalRead(pinPIR))
  {
    digitalWrite(pinBuzz, true);
    delay(500);
    digitalWrite(pinBuzz, false);
    delay(10);
  }
  sleep();
}
