#include "TimeWatcher.h"

void TimeWatcher::buzzIN()
{
    if (isItOn && ti.tm_sec == 0 && prevMinutesBuzzIN != ti.tm_min)
    {
        prevMinutesBuzzIN = ti.tm_min;
        if (ti.tm_min % 10 == 0)
        {
            for (BuzzData b : buzzes)
                if (b.minutes == ti.tm_min && b.minutes % buzzOnMin == 0)
                    buzzer->blink(b.itvBuzz, b.countBuzz);
        }
        else if (buzzOnMin == 5 && ti.tm_min % 5 == 0)
            buzzer->blink(100, 2);
    }
}
