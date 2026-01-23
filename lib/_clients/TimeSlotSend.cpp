#include "TimeSlotSend.h"

const char TimeSlotSend::cmdTime[] = "time";

int TimeSlotSend::secondsUntilNextSlot(int h, int m, int s) const
{
    // Convert current time to total seconds from start of day
    int now = h * 3600 + m * 60 + s;
    // Start searching from the next second
    int target = now;
    while (true)
    {
        int tm = (target / 60) % 60;
        int ts = target % 60;
        if ((tm % slotMin) == 0 && ts == slotSec)
            return target - now;
        target++;
    }
}

void TimeSlotSend::onTimeStringRecv(const uint8_t *data, int len, ulong currentMillis, bool printTime)
{
    // printf("onTimeStringRecv: len=%d\n", len);
    if (len == 8 && data[2] == ':' && data[5] == ':') // if it's response to time command - e.g. 16:25:01
    {
        char str[10];
        memcpy(str, data, len);
        str[len] = 0;
        if (printTime)
            printf("Received time string: %s\n", str);

        int h, m, s, secs = 0;
        if (sscanf(str, "%d:%d:%d", &h, &m, &s) != 3)
            return;
        secs = secondsUntilNextSlot(h, m, s);
        // printf("Seconds to next %d-minute mark + %d sec: %d\n", getSlotMin(), getSlotSec(), secs);
        msTimeToSendData = currentMillis + secs * 1000UL - itvSensorRead;
        msTimeReqSent = 0; // reset time request sent
        isWakeUpTimeWrong = secs > secWakeUpTimeWrong;
    }
}
