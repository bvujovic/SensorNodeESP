#pragma once

#include <stdio.h>
#include <string.h>

/// @brief Solution for sleep and timely reporting by clients. Client will sleep for e.g. 9 minutes
/// and then asks hub for current time and then sends data based on that time.
class TimeSlotSend
{
private:
    uint8_t slotMin;             // (minutes) Send data every slotMin minutes
    uint8_t slotSec;             // (seconds) After the slotMin-minute mark (e.g. 5): 11:00:05, 11:10:05...
    int16_t secBeforeWakeup;     // Seconds to wake up before the slot time. Can be negative if device wakes up too early.
    uint16_t itvSensorRead;      // (milliseconds) Time it takes to read data from sensors
    uint16_t secWakeUpTimeWrong; // (seconds) Threshold to consider wake-up time as off if more than this value to wait

    static const char cmdTime[];     // "time" command string
    uint16_t itvTimeRespWait = 5000; // (milliseconds) Time to wait for "time" response
    ulong msTimeReqSent = 0;         // Time when "time" request is sent
    ulong msTimeToSendData = 0;      // Time to send data from sensors
    bool isWakeUpTimeWrong = false;  // flag to indicate wake-up time is wrong (too early or too late)

public:
    /// @brief Construct a new Time Slot Send object
    /// @param slotMin Send data every slotMin minutes
    /// @param slotSec Seconds after the slotMin-minute mark (e.g. 5): 11:00:05, 11:10:05...
    /// @param secBeforeWakeup Seconds to wake up before the slot time. Can be negative if device wakes up too early.
    /// @param itvSensorRead (milliseconds) Time it takes to read data from sensors
    /// @param secWakeUpTimeWrong (seconds) Threshold to consider wake-up time as off if more than this value to wait
    TimeSlotSend(uint8_t slotMin, uint8_t slotSec, int16_t secBeforeWakeup, uint16_t itvSensorRead, uint16_t secWakeUpTimeWrong)
        : slotMin(slotMin), slotSec(slotSec), secBeforeWakeup(secBeforeWakeup), itvSensorRead(itvSensorRead), secWakeUpTimeWrong(secWakeUpTimeWrong)
    {
    }

    uint8_t getSlotMin() const { return slotMin; }
    uint8_t getSlotSec() const { return slotSec; }
    int16_t getSecBeforeWakeup() const { return secBeforeWakeup; }
    uint16_t getItvSensorRead() const { return itvSensorRead; }
    uint16_t getSecWakeUpTimeWrong() const { return secWakeUpTimeWrong; }

    const char *getCmdTime() const { return cmdTime; }
    bool getIsWakeUpTimeWrong() const { return isWakeUpTimeWrong; }
    // set isWakeUpTimeWrong to false
    void resetWakeUpTimeWrong() { isWakeUpTimeWrong = false; }
    // set itvTimeRespWait - Max time to wait for "time" response
    void setItvTimeRespWait(uint16_t itv) { itvTimeRespWait = itv; }

    // calculate seconds until next time slot (time to send data to the hub) from given h,m,s
    int secondsUntilNextSlot(int h, int m, int s) const;

    // TODO add param dt or dms to adjust msTimeToSendData (can be +/- and it's used if getting data from sensors takes long time)
    void onTimeStringRecv(const uint8_t *data, int len, ulong currentMillis, bool printTime);
    // calculate deep sleep time in microseconds
    uint64_t getDeepSleepTime() const
    {
        return (uint64_t)(slotMin * 60 - secBeforeWakeup) * 1000000UL;
    }
    // check if it's time to send data
    bool isTimeToSendData(ulong currentMillis) const
    {
        return msTimeToSendData != 0 && currentMillis > msTimeToSendData;
    }
    // set msTimeReqSent to currentMillis - signal that time request is sent
    void timeReqIsSent(ulong currentMillis)
    {
        msTimeReqSent = currentMillis;
        resetWakeUpTimeWrong();
    }
    // TODO add maxRetries for limiting number of retries
    // check if time response is missing - (itvTimeRespWait) milliseconds passed
    bool isTimeRespMissing(ulong currentMillis)
    {
        if (msTimeReqSent == 0)
            return false;
        return currentMillis > msTimeReqSent + itvTimeRespWait;
    }
};
