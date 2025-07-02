#pragma once

//TODO Pogledati https://stackoverflow.com/questions/11714325/how-to-get-enum-item-name-from-its-value
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
    UndefinedSensorType,
    /// @brief Simple notfication without any additional data
    SimpleEvent,
    /// @brief Data from ENS160+AHT21 module: temp, hum, ECO2, AQI...
    EnsAht,
    /// @brief Data from BMP280 sensor: temperature
    Temperature,
    /// @brief Data from ENS160 & DHT22 module: temp, hum, ECO2, AQI...
    EnsDht,
};

enum Device
{
    UndefinedDevice,
    TestNodeMCU,
    WemosExtAnt,
    Wemos1,
};

/// @brief Depending on event type hub will handle data differently: log data, send WA notification, buzz...
enum EventType
{
    Information,
    Error,
    Warning,
    Critical,
};
