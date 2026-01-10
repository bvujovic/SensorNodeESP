#include "SimpleEventHandler.h"

const char *SimpleEventHandler::unknownDevice = "Unknown device";

SimpleEventHandler::SimpleEventHandler()
{
    clearEventData();

    // list of known devices - peers from my_esp_now.h
    // devices.push_back(std::make_pair((const unsigned char *)"\x2C\xBC\xBB\x92\x1A\x34", "Kitchen/Sink"));

    //? testing: print first MAC address
    // const unsigned char *mac = devices[0].first;
    // printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void SimpleEventHandler::newMessage(char *msg, peer_info *p)
{
    // printf("SimpleEventHandler::newMessage called with msg='%s'\n", msg);
    // parsing msg
    unsigned long msgId;
    // int res = sscanf(msg, "%lu;%s", &msgId, &msgText);
    // if (res != 2)
    //     return; // invalid message format

    // find index of ; in msg and replace it with \0
    int idxSeparator = -1;
    for (size_t i = 0; msg[i] != '\0'; i++)
        if (msg[i] == ';')
        {
            idxSeparator = i;
            break;
        }
    if (idxSeparator == -1)
        return;
    msg[idxSeparator] = '\0';

    // parse msg from to msgId
    msgId = strtoul(msg, NULL, 10);
    msgText = msg + idxSeparator + 1;

    // if it's in msgIDs discard msg
    for (size_t i = 0; i < msgIDs.size(); i++)
        if (msgIDs[i] == msgId)
            return;
    msgIDs.push_back(msgId);

    // find device name from mac
    // deviceName = NULL;
    // for (size_t i = 0; i < devices.size(); i++)
    // {
    //     const unsigned char *dmac = devices[i].first;
    //     // TODO change for loop to if(mac[0]==dmac[0] && ...)
    //     bool match = true;
    //     for (size_t j = 0; j < 6; j++)
    //         if (mac[j] != dmac[j])
    //         {
    //             match = false;
    //             break;
    //         }
    //     if (match)
    //     {
    //         deviceName = (char *)devices[i].second;
    //         break;
    //     }
    // }
    // if (deviceName == NULL)
    //     deviceName = (char *)unknownDevice;
    peer = p;
    // TODO maybe a vector for multiple events that should be processed
    // printf("SimpleEventHandler::newMessage finished: deviceName='%s', msgText='%s'\n", deviceName, msgText);

}

void SimpleEventHandler::clearEventData()
{
    // msgText[0] = '\0';
    msgText = NULL;
    // deviceName = NULL;
    peer = NULL;
}
