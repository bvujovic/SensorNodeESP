//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04
//* ESP32 WROOM-32 test device (yellow pins) MAC Address: 30:AE:A4:47:9C:C4
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp8266-nodemcu/

#include <Arduino.h>
#include <esp_now.h>

// uint8_t macEsp32Dev[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
// bool isDataReceived;
esp_now_peer_info_t peerInfo;

// struct esp_now_peer11
// {
//     const char *name;
//     uint8_t testMac[6];
// };

// esp_now_peer_info peers[] =
// {
//     { .peer_addr = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4}, .channel = 0, .encrypt = false },
// };
// uint8_t macEsp32Dev[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};

esp_now_peer_info peers[2];
int cntPeers;
int lenMillisCommand;
esp_now_peer_info *peerRespMillis = NULL; // send millis to this peer
char cmdRequest[80];

void setPeers()
{
    lenMillisCommand = strlen("millis");
    cntPeers = sizeof(peers) / sizeof(esp_now_peer_info);

    // uint8_t macEsp32Dev[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    memcpy(peers[0].peer_addr, (const uint8_t[]){0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4}, 6);
    // char nameEsp32Dev[] = {'E', 'S', 'P', 0};
    // char nameEsp32Dev[] = "ESP32 dev";
    // Serial.println(strlen(nameEsp32Dev));
    // memcpy(peers[0].lmk, nameEsp32Dev, 2);
    strcpy((char *)peers[0].lmk, "ESP32 dev");
    peers[0].encrypt = peers[0].channel = 0;

    // My NodeMCU's MAC address: 84:F3:EB:77:04:BA
    memcpy(peers[1].peer_addr, (const uint8_t[]){0x84, 0xF3, 0xEB, 0x77, 0x04, 0xBA}, 6);
    strcpy((char *)peers[1].lmk, "NodeMCU");
    peers[1].encrypt = peers[1].channel = 0;
}

// // Structure example to send data
// // Must match the receiver structure
// typedef struct struct_message
// {
//     float temp;
//     float hum;
//     float pres;
// } struct_message;
// memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

// // Create a struct_message called BME280Readings to hold sensor readings
// struct_message BME280Readings;

bool equalMACs(const uint8_t *mac1, const uint8_t *mac2)
{
    for (size_t i = 0; i < 6; i++)
        if (mac1[0] != mac2[0])
            return false;
    return true;
}

esp_now_peer_info *findPeer(const uint8_t *mac)
{
    for (auto &&p : peers)
        if (equalMACs(p.peer_addr, mac))
            return &p;
    return NULL;
}

void printMAC(const uint8_t *mac)
{
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// esp_err_t addPeer()
// {
//     memcpy(peerInfo.peer_addr, macEsp32Dev, 6);
//     peerInfo.channel = 0;
//     peerInfo.encrypt = false;
//     auto res = esp_now_add_peer(&peerInfo);
//     if (res != ESP_OK)
//         Serial.printf("Failed to add peer: %X\n", res);
//     return res;
// }

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    auto p = findPeer(mac);
    if (p != NULL)
    {
        Serial.printf("Data received from: %s\n", p->lmk);
        if (len == lenMillisCommand)
        {
            // memcpy(cmdRequest, incomingData, len);
            // cmdRequest[len] = 0;
            // Serial.println(strcmp(cmdRequest, "millis")); // 0
            // Serial.println(strncmp((const char *)incomingData, "millis", lenMillisCommand)); // 0
            if (strncmp((const char *)incomingData, "millis", lenMillisCommand) == 0)
                peerRespMillis = p;
        }
    }
    else
        Serial.println("ESP-NOW peer unknown.");

    // if (equalMACs(mac, peers[0].peer_addr))
    //     Serial.println((char *)peers[0].lmk);
    // else
    //     printMAC(mac);

    // memcpy(macClient, mac, 6);

    // char req[80];
    // memcpy(req, incomingData, len);
    // req[len] = 0;
    // Serial.println(req);

    // Serial.print("Bytes received: ");
    // Serial.println(len);
}
