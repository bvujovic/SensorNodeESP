#include "NotifyWhatsApp.h"

int NotifyWhatsApp::sendMessage(const char *msg)
{
  //* https://www.callmebot.com/blog/free-api-whatsapp-messages/
  String url = "http://api.callmebot.com/whatsapp.php?";
  url = url + "phone=" + CMB_PHONE;
  url = url + "&text=" + msg;
  url = url + "&apikey=" + CMB_API_KEY;
  // Serial.println(url);
  WiFiClient wiFiClient;
  HTTPClient client;
  client.begin(wiFiClient, url);
  int respCode = client.GET();
  // Serial.printf("Resp code: %d\n", respCode);
  // if (respCode > 0)
  //     Serial.println(client.getString());
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
