#pragma once

// #include <Arduino.h>
#include "Enums.h"

typedef unsigned long ulong;

// TODO SrxParser object should be able to parse signals from multiple sensors (diff pulse intervals, pauses...)

/// @brief Parser for data/pulses comming from SRX882.
class SrxParser
{
private:
    /// @brief Number of pulses that will be sent by the client via stx822.
    /// @remarks This number should be 10, not 9.
    static const int cntExpectedPulses = 9;
    /// @brief (milliseconds) Expected pause between signals.
    static const int itvExpectedPause = 4000;
    /// @brief Counter for KitchenSinkWater pulses (5ms)
    int cntKitchenSinkWater = 0;
    /// @brief Time (msec) of the last received pulse.
    ulong msLastSignal = 0;
    /// @brief Time (msec) of the last interpreted command (sequence of pulses).
    ulong msLastCommand = 0;
    /// @brief Last interpreted command.
    SrxCommand cmdLast = None;

    /// @brief Counts pulses if they are the right length.
    /// @param pulse length of the pulse (milliseconds)
    /// @param ms current board time given by millis()
    void pulseCount(ulong pulse, ulong ms);
    /// @brief Is cnt equal (or close) to cntExpectedPulses.
    bool pulseCountOk(int cnt);
    /// @brief Sets cmdLast and msLastCommand to passed values.
    void setLastCommand(SrxCommand cmd, ulong ms);
    /// @brief Method is called from refresh() to reset several variables for new pulses/commands
    // before it returns given command.
    SrxCommand returnCommand(SrxCommand cmdClick, ulong ms);

public:
    /// @brief Checks if given pulses make some command - signal from some sensor with SRX882.
    /// @param pulse length of the pulse (milliseconds)
    /// @param ms current board time given by millis()
    SrxCommand refresh(ulong pulse, ulong ms);
};
