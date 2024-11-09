#include "Btn.h"

Btn::Btn(byte pin)
{
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP);
}

ClickType Btn::check(ulong ms)
{
    ClickType result = None;
    bool newState = digitalRead(pin);
    ulong itvPress = ms - msLastDebounce;
    if (newState != lastState)
    {
        if (newState) // button is released
        {
            state = newState;
            if (itvPress > itvLong)
                longClickReturned = false;
            else if (itvPress > itvShort)
                result = ShortClick;
        }
        msLastDebounce = ms;
    }
    else if (!newState) // button is pressed
        if (itvPress > itvLong && !longClickReturned)
        {
            result = LongClick;
            longClickReturned = true;
        }
    lastState = newState;
    return result;
}

bool Btn::isPressed()
{
    return !digitalRead(pin);
}
