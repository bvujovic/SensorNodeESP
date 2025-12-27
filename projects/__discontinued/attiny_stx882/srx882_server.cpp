//* ESP8266 gets pulses from SRX882 and blinks LED if there are e.g. 10 LOW pulses
//* of e.g. about 4900-5000 usec
//* code based on https://github.com/bvujovic/PingPongScore

#include <Arduino.h>
#include <ESP8266WiFi.h>

const byte pinIn = D7; // DATA on SRX882
const byte pinLed = LED_BUILTIN;

enum Command
{
    None,             // no command is detected
    KitchenSinkWater, //
};
int cntKitchenSinkWater = 0; // counter for KitchenSinkWater pulses (5ms)
ulong msLastSignal = 0;      // time (msec) of the last received pulse
ulong msLastCommand = 0;     // time (msec) of the last interpreted command (sequence of pulses)
Command cmdLast = None;      // last interpreted command

// is number of same pulses Ok: 10 +/-1
static bool pulseCountOk(int cnt)
{
    return cnt >= 9 && cnt <= 11;
}

void setLastCommand(Command cmd, ulong ms)
{
    cmdLast = cmd;
    msLastCommand = ms;
}

void pulseCount(ulong pulse, ulong ms)
{
    if (pulse > 4850 && pulse < 5000)
    {
        cntKitchenSinkWater++;
        msLastSignal = ms;
    }
}

Command returnCommand(Command cmdClick, ulong ms)
{
    cntKitchenSinkWater = 0;
    msLastSignal = 0;
    setLastCommand(cmdClick, ms);
    return cmdClick;
}

Command refresh(ulong pulse, ulong ms)
{
    Command result = None;
    pulseCount(pulse, ms);
    if (msLastSignal > 0 && ms > msLastSignal + 5)
    {
        if (pulseCountOk(cntKitchenSinkWater))
            return returnCommand(KitchenSinkWater, ms);
        cntKitchenSinkWater = 0;
        msLastSignal = 0;
    }
    return result;
}

void wiFiOff()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
}

void ledOn(bool isItOn)
{
    digitalWrite(pinLed, !isItOn);
}

void setup()
{
    wiFiOff();
    pinMode(pinIn, INPUT);
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.begin(115200);
}

void loop()
{
    ulong pulse = pulseIn(pinIn, LOW);
    Command cmd = refresh(pulse, millis());

    if (cmd != None)
    {
        ledOn(true);
        delay(500);
        ledOn(false);
    }
}
