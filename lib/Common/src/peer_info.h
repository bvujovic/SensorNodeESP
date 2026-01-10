#pragma once

#include <stdint.h>
#include <Enums.h>

#define MAC_LEN (6)

struct peer_info
{
    uint8_t peer_addr[MAC_LEN];
    SensorType type;
    Device device;
};
