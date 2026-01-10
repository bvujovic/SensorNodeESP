#include <Arduino.h>
#include <esp_now.h>
#include "MacAddresses.h"
#include "AirData.h"
#include "peer_info.h"
#include "Notification.h"
#include "ToString.h"

peer_info peers[5]; // ESP-NOW peers
int cntPeers;       // peers count

#define CMD_MILLIS ("millis")
int lenCmdMillis;                 // length of (string) "millis" command
peer_info *peerRespMillis = NULL; // send millis to this peer
#define CMD_TIME ("time")
int lenCmdTime; // length of (string) "time" command
// TODO this could be a class: CMD, lenCmd [peerResp]

void printMAC(const uint8_t *mac);
void addPeers();

void setPeer(peer_info *pi, uint8_t *mac, SensorType type, Device device)
{
    // Serial.println("Setting peer: ");
    // printMAC(mac);
    memcpy(pi->peer_addr, mac, 6);
    pi->type = type;
    pi->device = device;
}

void setPeers()
{
    lenCmdMillis = strlen(CMD_MILLIS);
    lenCmdTime = strlen(CMD_TIME);
    cntPeers = 0;
    setPeer(peers + (cntPeers++), macEsp8266WemosExtAnt, SensorType::SCD30, Device::WemosExtAnt);
    setPeer(peers + (cntPeers++), macEsp8266Wemos1, SensorType::EnsDht, Device::Wemos1);
    setPeer(peers + (cntPeers++), macEsp32Dev, SensorType::UndefinedSensorType, Device::ESP32DevKit);          // test ESP32
    setPeer(peers + (cntPeers++), macEsp8266NodeMCU, SensorType::UndefinedSensorType, Device::ESP8266NodeMCU); // test ESP8266
    setPeer(peers + (cntPeers++), macEsp32BattConnUsbC, SensorType::SimpleEvent, Device::ESP32BattConn);       // test SimpleEvent with ESP-NOW instead of STX882 or HC-12
    addPeers();
}

void addPeers()
{
    Serial.println("Adding ESP-NOW peers: ");
    // Serial.println(cntPeers);
    // Serial.println(LWIP_ARRAYSIZE(peers));
    for (auto &&p : peers)
    {
        // printMAC(p.peer_addr);
        auto peer = (esp_now_peer_info *)malloc(sizeof(esp_now_peer_info));
        peer->ifidx = WIFI_IF_AP;
        memcpy(peer->peer_addr, p.peer_addr, 6);
        peer->encrypt = false;
        peer->channel = 1;
        auto res = esp_now_add_peer(peer);
        if (res == ESP_OK)
            Serial.printf("\t%s\n", ToString::Devices[p.device]);
        else
            Serial.printf("- Failed to add peer. Reason: 0x%X\n", res);
    }
}

bool equalMACs(const uint8_t *mac1, const uint8_t *mac2)
{
    for (size_t i = 0; i < 6; i++)
        if (mac1[i] != mac2[i])
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

// discard batch messages (less than 1sec from the last one) from the same MAC address
bool burstDetected(const uint8_t *mac)
{
    static uint8_t lastMAC[MAC_LEN];
    static unsigned long lastTime = 0;
    unsigned long now = millis();
    static bool discardMsg = false;
    static int cntBursts = 1;
    if (equalMACs(mac, lastMAC) && (now - lastTime) < 1000)
    {
        discardMsg = true;
        cntBursts++;
    }
    else if (discardMsg) // end of burst
    {
        discardMsg = false;
        sprintf(line, "Burst detected (%d messages) from MAC address %02X:%02X:%02X:%02X:%02X:%02X"
            , cntBursts, lastMAC[0], lastMAC[1], lastMAC[2], lastMAC[3], lastMAC[4], lastMAC[5]);
        logger.add("HUB", "HUB", line); // log burst end
        cntBursts = 0;
    }
    memcpy(lastMAC, mac, MAC_LEN);
    lastTime = now;
    return discardMsg;
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (burstDetected(mac))
        return;

    auto p = findPeer(mac);
    if (p != NULL)
    {
        Serial.printf("Data received from %s @ %s, len: %d\n", ToString::SensorTypes[p->type], ToString::Devices[p->device], len);

        // response to ESP-NOW command/request: millis, time
        if (len == lenCmdMillis && strncmp((const char *)incomingData, CMD_MILLIS, lenCmdMillis) == 0)
            peerRespMillis = p;
        if (len == lenCmdTime && strncmp((const char *)incomingData, CMD_TIME, lenCmdTime) == 0)
        {
            getLocalTime(&ti);
            strftime(line, sizeof(line), "%H:%M:%S", &ti);
            esp_now_send(p->peer_addr, (uint8_t *)line, strlen(line));
            return;
        }
        // handling data from nodes (sensors)
        if (p->type == SensorType::EnsDht)
        {
            AirData ad;
            memcpy(&ad, incomingData, len);
            sprintf(line, "%.1f;%u;%u;%u;%u;%u", ad.temperature, ad.humidity, ad.status, ad.ECO2, ad.TVOC, ad.AQI);
            Serial.println(line);
            if (GetNotif(ECO2_1000)->buzz && ad.ECO2 >= 1000)
                buzzer.blinkWarning();
            if (GetNotif(AQI4)->buzz && ad.AQI == 4)
                buzzer.blinkWarning();
            if (GetNotif(AQI5)->buzz && ad.AQI == 5)
                buzzer.blinkCritical();
            logger.add(ToString::SensorTypes[p->type], ToString::Devices[p->device], line);
        }
        else if (p->type == SensorType::SCD30)
        {
            AirData ad;
            memcpy(&ad, incomingData, len);
            sprintf(line, "%.1f;%u;%u", ad.temperature, ad.humidity, ad.ECO2);
            Serial.println(line);
            logger.add(ToString::SensorTypes[p->type], ToString::Devices[p->device], line);
        }
        else if (p->type == SensorType::SimpleEvent)
        {
            memcpy(line, incomingData, len);
            line[len] = '\0';
            seh.newMessage(line, p);
        }
    }
    else
    {
        strncpy(line, (const char *)incomingData, len);
        line[len] = '\0';
        // printf("Data received from UNKNOWN peer: %s\n", line);
        // printMAC(mac);
        logger.add("Unknown Type", "Unknown Device", line);
    }
}
