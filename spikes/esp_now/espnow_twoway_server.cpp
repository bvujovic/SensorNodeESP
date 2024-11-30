//* ESP32 (w/ battery connector, micro USB) MAC Address: 30:C6:F7:04:66:04
//* ESP32 WROOM-32 test device (yellow pins) MAC Address: 30:AE:A4:47:9C:C4
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/
//* https://randomnerdtutorials.com/esp-now-two-way-communication-esp8266-nodemcu/

#include <esp_now.h>
#include <WiFi.h>

// #include <Wire.h>

// // Structure example to send data
// // Must match the receiver structure
// typedef struct struct_message
// {
//     float temp;
//     float hum;
//     float pres;
// } struct_message;

// // Create a struct_message called BME280Readings to hold sensor readings
// struct_message BME280Readings;

uint8_t macClient[6];
bool isDataReceived;
esp_now_peer_info_t peerInfo;

void printMAC(const uint8_t *mac)
{
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    // if (status == 0)
    //     success = "Delivery Success :)";
    // else
    //     success = "Delivery Fail :(";
}

esp_err_t addPeer()
{
    memcpy(peerInfo.peer_addr, macClient, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    auto res = esp_now_add_peer(&peerInfo);
    if (res != ESP_OK)
    {
        Serial.printf("Failed to add peer: %X\n", res);
        // Serial.println("Failed to add peer");
    }
    return res;
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    // memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    printMAC(mac);
    memcpy(macClient, mac, 6);

    char req[80];
    memcpy(req, incomingData, len);
    req[len] = 0;
    Serial.println(req);

    Serial.print("Bytes received: ");
    Serial.println(len);
    isDataReceived = true;

    // addPeer();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\nServer started!");
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        while (true)
            delay(100);
    }
    esp_now_register_send_cb(OnDataSent);

    uint8_t testMac[] = {0x30, 0xAE, 0xA4, 0x47, 0x9C, 0xC4};
    memcpy(peerInfo.peer_addr, testMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    auto res = esp_now_add_peer(&peerInfo);
    if (res != ESP_OK)
    {
        Serial.println("Failed to add peer");
        Serial.printf("Reason: %X\n", res);
        return;
    }
    else
        Serial.println("Client peer added.");

    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

char msg[10];

void loop()
{
    if (isDataReceived)
    {
        isDataReceived = false;
        Serial.print("Send to :");
        printMAC(macClient);
        ulong ms = millis();
        Serial.println(ms);
        ultoa(ms, msg, 10);
        esp_now_send(macClient, (uint8_t *)&msg, strlen(msg));
    }

    // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&BME280Readings, sizeof(BME280Readings));

    // if (result == ESP_OK)
    //     Serial.println("Sent with success");
    // else
    //     Serial.println("Error sending the data");

    delay(10);
}
