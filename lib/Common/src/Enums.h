#pragma once

// #define BANOVO_BRDO
#define VRANIC

// TODO Pogledati https://stackoverflow.com/questions/11714325/how-to-get-enum-item-name-from-its-value

/// @brief Type of sensor. What kind of data is sent/received: weather, room temp/CO2..., PIR/water detection...
/// This does not define particular device/sensor.
enum SensorType
{
  /// @brief Undefined sensor type, used for unknown devices
  UndefinedSensorType,
  /// @brief Simple notfication without any additional data
  SimpleEvent,
  /// @brief Data from ENS160+AHT21 module: temp, hum, ECO2, AQI... (AirData struct)
  EnsAht,

  /// @brief Temperature and humidity data from some sensor
  TempHumSensor,
  /// @brief Data from ENS160 & DHT22 module: temp, hum, ECO2, AQI... (AirData)
  EnsDht,
  /// @brief Data from BME680 module: temp, hum, ECO2, TVOC... (AirData)
  BME680,

  /// @brief Data from Sensirion SCD (SCD30, SCD41...) sensor: CO2, temp, hum... (AirData)
  SCD,
  /// @brief Number of sensor types
  SensorTypeCount
};

/// @brief Depending on device type hub will handle data differently: log data, send WA notification, buzz...
/// @see ToString::Devices to add or modify device names (as strings)
enum Device
{
  UndefinedDevice,
  ESP8266NodeMCU,
  WemosExtAnt,
  Wemos1,
  Wemos2,
  ESP32DevKit,
  ESP32BattConn,
  ESP32C3ProMini1,
  ESP32C3ant1,
  ESP32C3Xiao,
};

/// @brief Depending on event type hub will handle data differently: log data, send WA notification, buzz...
enum EventType
{
  Information,
  Error,
  Warning,
  Critical,
};

enum EnumNotification
{
  WaterDetected,
  MovementDetected,
  CO2_800,
  AQI4,
  ECO2_1000,
  AQI5,
};
