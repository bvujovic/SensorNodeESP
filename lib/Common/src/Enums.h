#pragma once

/// @brief
enum SrxCommand
{
    /// @brief no command is detected
    None,
    /// @brief water overflow in kitchen sink
    KitchenSinkWater,
};

/// @brief Type of sensor. What kind of data is sent/received: weather, room temp/CO2..., PIR/water detection...
/// This does not define particular device/sensor.
enum SensorType
{
    //? Undefined,
    /// @brief Simple notfication without any additional data
    SimpleEvent,
    /// @brief Data from ENS160+AHT21 module
    RoomTempAirQ
};

/// @brief Depending on event type hub will handle data differently: log data, send WA notification, buzz...
enum EventType
{
    Information,
    Error,
    Warning,
    Critical,
};
