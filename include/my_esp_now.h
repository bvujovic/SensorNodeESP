#include <Arduino.h>
#include <esp_now.h>
#include "MacAddresses.h"
#include "AirData.h"

const char *StrSensorTypes[] = {
    "Undefined",
    "SimpleEvent",
    "EnsAht",
    "Temp",
    "EnsDht",
    "BME680",
};

const char *SensorTypesComment[] = {
    "/",
    "Logged event without additional data",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Temperature: temp (C)",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC",
};

const char *StrDevices[] = {
    "Undefined",
    "TestNodeMCU",
    "WemosExtAnt",
    "ESP8266 Wemos 01",
    "ESP32DevKit",
};

struct Notification
{
    int id;
    String name;
    bool buzz;
    bool wa_msg;
};

enum EnumNots
{
    WaterDetected,
    AQI4,
    ECO2_1000,
    AQI5,
};

Notification notifications[] = {
    {WaterDetected, "Water detected", 1, 1},
    {AQI4, "Air quality: AQI >= 4", 0, 0},
    {ECO2_1000, "Air quality: ECO2 >= 1000", 0, 0},
    {AQI5, "Air quality: AQI >= 5", 0, 0},
};

/// @brief Gets the notification, given its id (EnumNots).
Notification *GetNotif(EnumNots e)
{
    for (auto &&n : notifications)
        if (n.id == e)
            return &n;
    return NULL;
}

struct peer_info
{
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];
    SensorType type;
    Device device;
};

peer_info peers[2];               // ESP-NOW peers
int cntPeers;                     // peers count
int lenMillisCommand;             // length of (string) "millis" command
peer_info *peerRespMillis = NULL; // send millis to this peer

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
    lenMillisCommand = strlen("millis");
    // cntPeers = sizeof(peers) / sizeof(esp_now_peer_info);
    cntPeers = 0;
    // memcpy(peers[0].peer_addr, macEsp32Dev, 6);
    //* My NodeMCU's MAC address: 84:F3:EB:77:04:BA    It works with these settings:
    //* https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
    //* WiFi.softAPmacAddress is created from WiFi.macAddress by adding 1 to the last byte
    // uint8_t mac[] = {0x30, 0xC6, 0xF7, 0x04, 0x66, 0x05};
    // esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    setPeer(peers + (cntPeers++), macEsp8266Wemos1, SensorType::EnsDht, Device::Wemos1);
    setPeer(peers + (cntPeers++), macEsp32Dev, SensorType::BME680, Device::ESP32DevKit);
    setPeer(peers + (cntPeers++), macEsp8266NodeMCU, SensorType::SCD30, Device::WemosExtAnt);
    //* When adding more peers, don't forget to update size of peers[] array.
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
            Serial.printf("\t%s\n", StrDevices[p.device]);
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
        Serial.printf("Data received from %s @ %s\n", StrSensorTypes[p->type], StrDevices[p->device]);
        if (len == lenMillisCommand && strncmp((const char *)incomingData, "millis", lenMillisCommand) == 0)
            peerRespMillis = p;

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
            logger.add(StrSensorTypes[p->type], StrDevices[p->device], line);
        }
        if (p->type == SensorType::BME680)
        {
            AirData ad;
            memcpy(&ad, incomingData, len);
            sprintf(line, "%.1f;%u;%u;%u;%u", ad.temperature, ad.humidity, ad.status, ad.ECO2, ad.TVOC);
            Serial.println(line);
            logger.add(StrSensorTypes[p->type], StrDevices[p->device], line);
        }
        if (p->type == SensorType::Temperature)
        {
            float temp;
            memcpy(&temp, incomingData, len);
            sprintf(line, "%.2f", temp);
            // Serial.println(len);
            // Serial.println(temp);
            logger.add(StrSensorTypes[p->type], StrDevices[p->device], line);
        }
    }
    else
    {
        Serial.println("ESP-NOW peer unknown.");
        printMAC(mac);
    }
}
