#pragma once

#include <WString.h>

struct Notification
{
    int id;
    String name;
    bool buzz;
    bool wa_msg;
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
