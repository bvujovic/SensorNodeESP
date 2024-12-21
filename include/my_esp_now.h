//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04
//* ESP32 WROOM-32 test device (yellow pins) MAC Address: 30:AE:A4:47:9C:C4
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp8266-nodemcu/

#include <Arduino.h>
#include <esp_now.h>

// B esp_now_peer_info_t peerInfo;

esp_now_peer_info peers[2];               // ESP-NOW peers
int cntPeers;                             // peers count
int lenMillisCommand;                     // length of (string) "millis" command
esp_now_peer_info *peerRespMillis = NULL; // send millis to this peer
char cmdRequest[80];

void addPeers();

void setPeers()
{
    lenMillisCommand = strlen("millis");
    cntPeers = sizeof(peers) / sizeof(esp_now_peer_info);

    // uint8_t macEsp32Dev[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    memcpy(peers[0].peer_addr, (const uint8_t[]){0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4}, 6);
    strcpy((char *)peers[0].lmk, "ESP32 dev");
    peers[0].encrypt = peers[0].channel = 0;

    //* My NodeMCU's MAC address: 84:F3:EB:77:04:BA    It works with these settings:
    //* https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
    //* WiFi.softAPmacAddress is created from WiFi.macAddress by adding 1 to the last byte
    // uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
    // esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    memcpy(peers[1].peer_addr, (const uint8_t[]){0x84, 0xF3, 0xEB, 0x77, 0x04, 0xBA}, 6);
    strcpy((char *)peers[1].lmk, "NodeMCU");
    peers[1].encrypt = peers[1].channel = 0;

    addPeers();
}

void addPeers()
{
    Serial.println("Adding ESP-NOW peers: ");
    for (auto &&p : peers)
    {
        auto res = esp_now_add_peer(&p);
        if (res != ESP_OK)
        {
            Serial.println("- Failed to add peer");
            Serial.printf("- Reason: %X\n", res);
        }
        else
            Serial.printf("\t%s\n", p.lmk);
    }
}

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

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    auto p = findPeer(mac);
    if (p != NULL)
    {
        Serial.printf("Data received from: %s\n", p->lmk);
        if (len == lenMillisCommand && strncmp((const char *)incomingData, "millis", lenMillisCommand) == 0)
            peerRespMillis = p;
    }
    else
        Serial.println("ESP-NOW peer unknown.");
}
