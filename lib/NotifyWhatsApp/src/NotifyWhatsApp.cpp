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
