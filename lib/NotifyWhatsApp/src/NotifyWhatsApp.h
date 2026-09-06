#pragma once

#include <HTTPClient.h>
#include <CredCallMeBot.h>

// CallMeBot WhatsApp API: OK constant
#define CMB_OK (200)

// Used for logging purposes to identify the type of log messages
#define CMB_LOG_TYPE "NotifyWhatsApp"

class NotifyWhatsApp
{
private:
public:
  static int sendMessage(const char *msg);
  // static String &errorMessage(int errorCode);
  static const char *errorMessage(int errorCode);
};
