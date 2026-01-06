#pragma once

#include <stdio.h>
#include <vector>

class SimpleEventHandler
{
private:
    static const char *unknownDevice; // default name for unknown devices
    char *deviceName;                 // pointer to device name found from mac address
    char msgText[80];                 // buffer for message text

    std::vector<unsigned long> msgIDs;                                   // vector of processed message IDs
    std::vector<std::pair<const unsigned char *, const char *>> devices; // key-value pairs: mac address -> device name

public:
    SimpleEventHandler(/* args */);
    bool newMessage(const char *msg, const unsigned char *mac);
    const char *getDeviceName() const { return deviceName; }
    const char *getMessageText() const { return msgText; }
    void EventHandled();
};
