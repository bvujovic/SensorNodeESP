#pragma once

#include <stdint.h>
#include <Enums.h>
// #include <WString.h>

#define MAC_LEN (6)

// struct Notification
// {
//     int id;
//     String name;
//     bool buzz;
//     bool wa_msg;
// };

struct peer_info
{
    uint8_t peer_addr[MAC_LEN];
    SensorType type;
    Device device;
};


// enum EnumNots
// {
//     WaterDetected,
//     AQI4,
//     ECO2_1000,
//     AQI5,
// };