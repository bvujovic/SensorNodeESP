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

#define CNT_REPEAT_SEND 3         // How many times signal is sent.
#define ITV_PAUSE 2               // (seconds) Pause between sending signals.
#define ITV_INIT_WAIT 5           // (seconds) Initial wait before going to sleep and start listening for interrupt.
#define SENSOR_NAME "KitchenSink" // Name of the sensor, used in the message.

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

    // Delay is needed for PIR sensor.
    // Otherwise we would trigger interrupt again immediately after waking up.
    for (int i = 0; i < ITV_INIT_WAIT * 1000; i++)
        delayMicroseconds(1000);

    pinMode(PB2, INPUT_PULLUP);

    // Enable INT0 interrupt on LOW
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
    // sprintf(buff, " Test:%03d.\n", i % 1000);
    HC12.write(SENSOR_NAME);
    HC12.write('\n');
    // HC12.write(buff);
}

void loop()
{
    //?pinMode(PIN_TX, INPUT); // Set PIN_TX to INPUT in order to save power.
    sei(); // Enable interrupts again, go to sleep and wait for intterrupt.
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
