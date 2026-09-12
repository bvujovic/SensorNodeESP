#include "NotifyWhatsApp.h"

// More robust solution: curl_easy_escape() (from libcurl): If you are building 
// standard web/HTTP query strings, libcurl provides curl_easy_escape(), 
// which converts spaces to %20 or + along with encoding all other unsafe URL characters
void NotifyWhatsApp::spaceToPlus(char *str)
{
  while (*str != '\0')
  {
    if (*str == ' ')
      *str = '+';
    str++;
  }
}

int NotifyWhatsApp::sendMessage(const char *msg)
{
  return _sendMessage((char *)msg);
}

int NotifyWhatsApp::sendMessage(char *msg)
{
  spaceToPlus(msg);
  return _sendMessage(msg);
}

int NotifyWhatsApp::_sendMessage(char *msg)
{
  //* https://www.callmebot.com/blog/free-api-whatsapp-messages/
  String url = "http://api.callmebot.com/whatsapp.php?";
  url = url + "phone=" + CMB_PHONE;
  url = url + "&text=" + msg;
  url = url + "&apikey=" + CMB_API_KEY;
  Serial.println(url);
  WiFiClient wiFiClient;
  HTTPClient client;
  client.begin(wiFiClient, url);
  int respCode = client.GET();
  Serial.printf("Resp code: %d\n", respCode);
  if (respCode > 0)
    Serial.println(client.getString());
  client.end();
  return respCode;
}

// String &NotifyWhatsApp::errorMessage(int errorCode)
const char *NotifyWhatsApp::errorMessage(int errorCode)
{
  // return (String("WhatsApp message sent, error resp code: ") + errorCode).c_str();
  static char errorMsg[100];
  sprintf(errorMsg, "WhatsApp message sent, error resp code: %d", errorCode);
  return errorMsg;
}
