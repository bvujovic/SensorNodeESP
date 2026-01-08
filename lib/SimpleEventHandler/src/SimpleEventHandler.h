#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <esp-now-defs.h>

class SimpleEventHandler
{
private:
    static const char *unknownDevice; // default name for unknown devices

    char *deviceName; // device name of the senders message
    char *msgText;    // message text
    // char msgText[80]; // buffer for message text
    peer_info *peer;

    std::vector<unsigned long> msgIDs;                                   // vector of processed message IDs
    std::vector<std::pair<const unsigned char *, const char *> > devices; // key-value pairs: mac address -> device name

public:
    SimpleEventHandler(/* args */);
    void newMessage(const unsigned char *mac, char *msg, peer_info *p);
    const char *getDeviceName() const { return deviceName; }
    const char *getMessageText() const { return msgText; }
    peer_info *getPeerInfo() const { return peer; }
    void clearEventData();                                           // resets event data
    bool isNewMessageReceived() const { return deviceName != NULL; } // checks if a new message was received
};
