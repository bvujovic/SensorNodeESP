#pragma once

#include <HTTPClient.h>
#include <CredCallMeBot.h>

class NotifyWhatsApp
{
private:
public:
    static int sendMessage(const char *msg);
};
