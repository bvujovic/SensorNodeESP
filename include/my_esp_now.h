#include <Arduino.h>
#include <esp_now.h>
#include "MacAddresses.h"
#include "AirData.h"

const char *StrSensorType[] = {
    "Undefined",
    "SimpleEvent",
    "EnsAht",
};

const char *StrDevice[] = {
    "Undefined",
    "TestNodeMCU",
};

struct peer_info
{
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];
    SensorType type;
    Device device;
};

peer_info peers[4];               // ESP-NOW peers
int cntPeers;                     // peers count
int lenMillisCommand;             // length of (string) "millis" command
peer_info *peerRespMillis = NULL; // send millis to this peer
char line[80];                    // general purpose string - formating data

void printMAC(const uint8_t *mac);
void addPeers();

void setPeer(peer_info *pi, uint8_t *mac, SensorType type, Device device)
{
    memcpy(pi->peer_addr, mac, 6);
    pi->type = type;
    pi->device = device;
}

void setPeers()
{
    lenMillisCommand = strlen("millis");
    cntPeers = sizeof(peers) / sizeof(esp_now_peer_info);

    // uint8_t macEsp32Dev[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    // B memcpy(peers[0].peer_addr, (const uint8_t[]){0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4}, 6);
    memcpy(peers[0].peer_addr, macEsp32Dev, 6);
    // strcpy((char *)peers[0].lmk, "ESP32 dev");
    // peers[0].encrypt = peers[0].channel = 0;

    //* My NodeMCU's MAC address: 84:F3:EB:77:04:BA    It works with these settings:
    //* https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
    //* WiFi.softAPmacAddress is created from WiFi.macAddress by adding 1 to the last byte
    // uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
    // esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    // B memcpy(peers[1].peer_addr, (const uint8_t[]){0x84, 0xF3, 0xEB, 0x77, 0x04, 0xBA}, 6);

    // memcpy(peers[1].peer_addr, macEsp8266NodeMCU, 6);
    // peers[1].type = SensorType::EnsAht;
    // peers[1].device = Device::TestNodeMCU;
    setPeer(peers + 1, macEsp8266NodeMCU, SensorType::EnsAht, Device::TestNodeMCU);

    // strcpy((char *)peers[1].lmk, "NodeMCU");
    // peers[1].encrypt = false;
    // peers[1].channel = 1;

    // B memcpy(peers[2].peer_addr, (const uint8_t[]){0x40, 0xF5, 0x20, 0x3E, 0xD5, 0x11}, 6);
    memcpy(peers[2].peer_addr, macEsp8266WemosExtAnt, 6);
    peers[2].type = SensorType::EnsAht;
    // strcpy((char *)peers[2].lmk, "Wemos ExtAnt");
    // peers[2].encrypt = peers[2].channel = 0;

    // B memcpy(peers[3].peer_addr, (const uint8_t[]){0x24, 0xEC, 0x4A, 0xC8, 0xA7, 0x2C}, 6);
    memcpy(peers[3].peer_addr, macEsp32C3, 6);
    // strcpy((char *)peers[3].lmk, "ESP32-C3");
    // peers[3].encrypt = peers[3].channel = 0;

    addPeers();
}

void addPeers()
{
    Serial.println("Adding ESP-NOW peers: ");
    for (auto &&p : peers)
    {
        // auto res = esp_now_add_peer(&p);
        auto peer = (esp_now_peer_info *)malloc(sizeof(esp_now_peer_info));
        // esp_now_peer_info_t peer = {};
        // peer = {};
        peer->ifidx = WIFI_IF_AP;
        memcpy(peer->peer_addr, p.peer_addr, 6);
        peer->encrypt = false;
        peer->channel = 1;
        // B printMAC(peer->peer_addr);
        auto res = esp_now_add_peer(peer);
        if (res == ESP_OK)
            Serial.printf("\t%s\n", StrDevice[p.device]);
        else
            Serial.printf("- Failed to add peer. Reason: 0x%X\n", res);
    }
}

bool equalMACs(const uint8_t *mac1, const uint8_t *mac2)
{
    for (size_t i = 0; i < 6; i++)
        if (mac1[0] != mac2[0])
            return false;
    return true;
}

peer_info *findPeer(const uint8_t *mac)
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
        Serial.printf("Data received from %s @ %s\n", StrSensorType[p->type], StrDevice[p->device]);
        if (len == lenMillisCommand && strncmp((const char *)incomingData, "millis", lenMillisCommand) == 0)
            peerRespMillis = p;

        if (p->type == SensorType::EnsAht)
        {
            AirData ad;
            memcpy(&ad, incomingData, len);
            sprintf(line, "%d;%u;%u;%u;%u;%u", ad.temperature, ad.humidity, ad.status, ad.ECO2, ad.TVOC, ad.AQI);
            Serial.println(line);
            if (ad.AQI >= 3)
                buzzer.blinkWarning();
            logger.add(StrSensorType[p->type], StrDevice[p->device], line);
        }
    }
    else
        Serial.println("ESP-NOW peer unknown.");
}
