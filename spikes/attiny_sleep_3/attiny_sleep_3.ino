//* https://www.re-innovation.co.uk/docs/sleep-modes-on-attiny85/
/*
 ATtiny85 sleep mode test

 Overview:

 This code reads in analog voltage, outputs a Fast PWM and switches ON/OFF an LED.
 I want to investigate sleep modes to see power consumption and savings.
 This is to reduce the energy consumed so it can be attached to a lead acid battery.

 This code is now fully powered down and only wakes up with the watchdog timer

 This code is designed to run on the ATTiny 25/45/85
 The serial output only works with the larger ATTiny85 IC

 The connections to the ATTiny are as follows:
 ATTiny    Arduino    Info
 Pin  1  - 5          RESET / Rx (Not receiving any data)
 Pin  2  - 3          Tx for serial conenction
 Pin  3  - 4          FET driver (PWM)
 Pin  4  -            GND
 Pin  5  - 0          RED LED (PWM)
 Pin  6  - 1          GREEN LED
 Pin  7  - 2 / A1     Vsensor (Analog)
 Pin  8  -   +Vcc

 See www.re-innovation.co.uk for more details including flow code

 14/11/13 by Matt Little (matt@re-innovation.co.uk/www.re-innovation.co.uk)

 Added code from (http://www.insidegadgets.com/2011/02/05/reduce-attiny-power-consumption-by-sleeping-with-the-watchdog-timer/).
 Thanks to:
 * KHM 2008 / Lab3/  Martin Nawrath nawrath@khm.de
 * Kunsthochschule fuer Medien Koeln
 * Academy of Media Arts Cologne
 *
 * Modified on 5 Feb 2011 by InsideGadgets (www.insidegadgets.com)
 * to suit the ATtiny85 and removed the cbi( MCUCR,SE ) section
 * in setup() to match the Atmel datasheet recommendations

 Updated:
 14/11/13   - Added Power_Down Sleep mode - Matt Little


 This example code is in the public domain.
 */

#define F_CPU 8000000 // This is used by delay.h library

#include <stdlib.h>
#include <EEPROM.h>

#include <avr/io.h>     // Adds useful constants
#include <util/delay.h> // Adds delay_ms and delay_us functions

#include <avr/sleep.h>
#include <avr/interrupt.h>

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// *********** Define I/O Pins **********************
// LED output pins:
const int redled = 0;    // Red LED attached to here (0, IC pin 5)
const int buzzLedSw = 1; // Green LED/buzzer/Switch attached to here (1, IC pin 6)
// MOSFET Driver output
const int FETdriver = 4;
// Analog sensing pin
const int VsensePin = A1; // Reads in the analogue number of voltage
// Only use Serial if using ATTiny85
// Serial output connections:
#include <SoftwareSerial.h>
#define rxPin 5 // We use a non-existant pin as we are not interested in receiving data
#define txPin 3
SoftwareSerial serial(rxPin, txPin);

#define INTERNAL2V56NC (6) // We use the internal voltage reference

//************ USER PARAMETERS***********************
//**********MODE*************************************
const int deviceType = 85; // 45 = ATTiny45, NO serial output, 85 = AtTiny85, with serial output

// Variables
unsigned long int averageSensor = 0; // This holds the average value of the sensor

// Varibales for EEPROM
int hiByte; // These are used to store longer variables into EEPROM
int loByte;

// Varibles for the calibration factor
int calibrationFactor = 0; // This holds the Vref value in millivolts

// Variables for the Sleep/power down modes:
volatile boolean f_wdt = 1;

// the setup routine runs once when you press reset:
void setup()
{

    // Set up FAST PWM
    TCCR0A = 2 << COM0A0 | 2 << COM0B0 | 3 << WGM00; // Set control register A for Timer 0
    TCCR0B = 0 << WGM02 | 1 << CS00;                 // Set control register B for Timer 0
    TCCR1 = 0 << PWM1A | 0 << COM1A0 | 1 << CS10;    // Set control register for Timer 1
    GTCCR = 1 << PWM1B | 2 << COM1B0;                // General control register for Timer 1

    // Set up IO pins
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    pinMode(redled, OUTPUT);
    pinMode(buzzLedSw, OUTPUT); // First want to read the switch
    pinMode(FETdriver, OUTPUT);

    digitalWrite(FETdriver, LOW); //  Switch the FET OFF

    if (deviceType == 85)
    {
        // Start the serial output string - Only for ATTiny85 Version
        serial.begin(4800);
        serial.println("SLEEP ATTiny85");
        serial.println("14/11/13 Matt Little");
    }

    analogReference(INTERNAL2V56NC); // This sets the internal ref to be 2.56V (or close to this)
    delay(100);                      // Wait a while for this to stabilise.

    // Read in the Calibration Factor
    hiByte = EEPROM.read(124);
    loByte = EEPROM.read(125);

    calibrationFactor = (hiByte << 8) + loByte; // Get the sensor calibrate value

    serial.print("Calibration Factor: "); // TESTING
    serial.println(calibrationFactor);    // TESTING

    // Set the Fast PWM output for the Red LED
    analogWrite(redled, 127);    // Set it to 50%  running at 31.2kHz
    analogWrite(FETdriver, 127); // Set it to 50%  running at 31.2kHz

    setup_watchdog(8); // approximately 0.5 seconds sleep
}

// the loop routine runs over and over again forever:
void loop()
{

    if (f_wdt == 1)
    {              // wait for timed out watchdog / flag is set when a watchdog timeout occurs
        f_wdt = 0; // reset flag

        digitalWrite(buzzLedSw, HIGH); // GREEN LED ON
        _delay_ms(500);                // Switch the LED on for 0.5 Seconds

        averageSensor = analogRead(VsensePin);
        averageSensor = (averageSensor * calibrationFactor) / 1024; // This is the int of the real voltage
        serial.print("Analog Voltage reading: ");
        serial.println(averageSensor);

        digitalWrite(buzzLedSw, LOW); // Green LED OFF

        // Set the ports to be inputs - saves more power
        pinMode(txPin, INPUT);
        pinMode(redled, INPUT);
        pinMode(buzzLedSw, INPUT); // First want to read the switch
        pinMode(FETdriver, INPUT);

        system_sleep(); // Send the unit to sleep

        // Set the ports to be output again
        pinMode(rxPin, INPUT);
        pinMode(txPin, OUTPUT);
        pinMode(redled, OUTPUT);
        pinMode(buzzLedSw, OUTPUT); // First want to read the switch
        pinMode(FETdriver, OUTPUT);
    }
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep()
{

    cbi(ADCSRA, ADEN); // switch Analog to Digitalconverter OFF

    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
    sleep_enable();

    sleep_mode(); // System actually sleeps here

    sleep_disable(); // System continues execution here when watchdog timed out

    sbi(ADCSRA, ADEN); // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii)
{

    byte bb;
    int ww;
    if (ii > 9)
        ii = 9;
    bb = ii & 7;
    if (ii > 7)
        bb |= (1 << 5);
    bb |= (1 << WDCE);
    ww = bb;

    MCUSR &= ~(1 << WDRF);
    // start timed sequence
    WDTCR |= (1 << WDCE) | (1 << WDE);
    // set new watchdog timeout value
    WDTCR = bb;
    WDTCR |= _BV(WDIE);
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect)
{
    f_wdt = 1; // set global flag
}
