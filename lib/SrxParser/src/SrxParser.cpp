#include "SrxParser.h"

void SrxParser::pulseCount(ulong pulse, ulong ms)
{
    //TODO check if this interval should be changed (narrowed)
    // if (pulse > 4850 && pulse < 5000)
    if (pulse > 4900 && pulse < 5000) //! temporary change 
    {
        cntKitchenSinkWater++;
        msLastSignal = ms;
    }
}

bool SrxParser::pulseCountOk(int cnt)
{
    //TODO check if the value of cntExpectedPulses can be lowered - maybe system will work reliably with 5 pulses
    // return cnt >= cntExpectedPulses - 1 && cnt <= cntExpectedPulses + 1;
    return cnt == 3; //! temporary change
}

SrxCommand SrxParser::returnCommand(SrxCommand cmdClick, ulong ms)
{
    cntKitchenSinkWater = 0;
    msLastSignal = 0;
    setLastCommand(cmdClick, ms);
    return cmdClick;
}

void SrxParser::setLastCommand(SrxCommand cmd, ulong ms)
{
    cmdLast = cmd;
    msLastCommand = ms;
}

SrxCommand SrxParser::refresh(ulong pulse, ulong ms)
{
    SrxCommand result = None;
    pulseCount(pulse, ms);
    if (msLastSignal > 0 && ms > msLastSignal + 5)
    {
        if (pulseCountOk(cntKitchenSinkWater))
        {
            auto itvPause = ms - msLastCommand;
            if (itvPause < itvExpectedPause - 100 || itvPause > itvExpectedPause + 100)
                return returnCommand(KitchenSinkWater, ms);
            else
                return returnCommand(None, ms);
        }
        cntKitchenSinkWater = 0;
        msLastSignal = 0;
    }
    return result;
}
