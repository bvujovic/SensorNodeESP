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
        // char str[10];
        memcpy(strTime, data, len);
        strTime[len] = 0;
        if (printTime)
            printf("Received time string: %s\n", strTime);

        int h, m, s, secs = 0;
        if (sscanf(strTime, "%d:%d:%d", &h, &m, &s) != 3)
            return;
        secs = secondsUntilNextSlot(h, m, s);
        // printf("Seconds to next %d-minute mark + %d sec: %d\n", getSlotMin(), getSlotSec(), secs);
        msTimeToSendData = currentMillis + secs * 1000UL - itvSensorRead;
        msTimeReqSent = 0; // reset time request sent
        isWakeUpTimeWrong = secs > secWakeUpTimeWrong;
        itvWrongTimeDiff = isWakeUpTimeWrong ? secs - secBeforeWakeup : UINT16_MAX;
    }
}

const char *TimeSlotSend::getCurrentTime(ulong currentMillis) const
{
    // Return current time string if it's received, otherwise return empty string
    if (strTime[0] == 0)
        return "";
    // Calculate current time based on strTime, msTimeReqSent and currentMillis
    int h, m, s;
    if (sscanf(strTime, "%d:%d:%d", &h, &m, &s) != 3)
        return "";
    ulong startSeconds = h * 3600 + m * 60 + s;
    ulong ms = startSeconds * 1000 + currentMillis - msTimeReqSent;
    int seconds = ms / 1000;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    static char buf[12];
    sprintf(buf, "%02d:%02d:%02d", hours, minutes, secs);
    return buf;
}
