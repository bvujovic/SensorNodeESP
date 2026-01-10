#pragma once

//TODO Pogledati https://stackoverflow.com/questions/11714325/how-to-get-enum-item-name-from-its-value

/// @brief Type of sensor. What kind of data is sent/received: weather, room temp/CO2..., PIR/water detection...
/// This does not define particular device/sensor.
enum SensorType
{
    UndefinedSensorType,
    /// @brief Simple notfication without any additional data
    SimpleEvent,
    /// @brief Data from ENS160+AHT21 module: temp, hum, ECO2, AQI... (AirData struct)
    EnsAht,
    /// @brief Data from BMP280 sensor: temperature
    Temperature,
    /// @brief Data from ENS160 & DHT22 module: temp, hum, ECO2, AQI... (AirData)
    EnsDht,
    /// @brief Data from BME680 module: temp, hum, ECO2, TVOC... (AirData)
    BME680,
    /// @brief Data from SCD30 sensor: CO2, temp, hum... (AirData)
    SCD30,
    SensorTypeCount // number of sensor types
};

// const char *StrSensorTypes[] = {
//     "Undefined",
//     "SimpleEvent",
//     "EnsAht",
//     "Temp",
//     "EnsDht",
//     "BME680",
//     "SCD30",
// };

// const char *SensorTypesComment[] = {
//     "/",
//     "Logged event without additional data",
//     "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
//     "Temperature: temp (C)",
//     "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
//     "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC",
//     "Air quality: temp (C), hum (%), CO2 (ppm)",
// };

enum Device
{
    UndefinedDevice,
    ESP8266NodeMCU,
    WemosExtAnt,
    Wemos1,
    ESP32DevKit,
    ESP32BattConn,
};

// const char *StrDevices[] = {
//     "Undefined",
//     "ESP8266 NodeMCU",
//     "WemosExtAnt",
//     "ESP8266 Wemos 01",
//     "ESP32 DevKit",
//     "Kitchen/Sink", // "ESP32 BattConn",
// };

/// @brief Depending on event type hub will handle data differently: log data, send WA notification, buzz...
enum EventType
{
    Information,
    Error,
    Warning,
    Critical,
};

enum EnumNots
{
    WaterDetected,
    AQI4,
    ECO2_1000,
    AQI5,
};
