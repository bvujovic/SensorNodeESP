#include "ToString.h"

const char *ToString::SensorTypes[] = {
    "Undefined",
    "SimpleEvent",
    "EnsAht",
    "Temp",
    "EnsDht",
    "BME680",
    "SCD30",
};

const char *ToString::SensorTypesComment[] = {
    "/",
    "Logged event without additional data",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Temperature: temp (C)",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC",
    "Air quality: temp (C), hum (%), CO2 (ppm)",
};

const char *ToString::Devices[] = {
    "Undefined",
    "ESP8266 NodeMCU",
    "WemosExtAnt",
    "ESP8266 Wemos 01",
    "ESP32 DevKit",
    "Kitchen/Sink", // "ESP32 BattConn",
};
