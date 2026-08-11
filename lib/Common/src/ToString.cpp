#include "ToString.h"

const char *ToString::SensorTypes[] = {
    "Undefined",
    "SimpleEvent",
    "EnsAht",
    "TempHum",
    "EnsDht",
    "BME680",
    "SCD30",    
};

const char *ToString::SensorTypesComment[] = {
    "/",
    "Logged event without additional data",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Temperature and humidity: temp (C), hum (%)",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC, AQI",
    "Air quality: temp (C), hum (%), status, eqCO2 (ppm), TVOC",
    "Air quality: temp (C), hum (%), CO2 (ppm)",
};

const char *ToString::Devices[] = {
    "Undefined",
    "ESP8266 NodeMCU",
    "Main.Desk.CO2",        // "WemosExtAnt"
    "Main.Desk.Air",        // "ESP8266 Wemos 01"
    "Main.Desk.CO2",        // "ESP8266 Wemos 02"
    "ESP32 DevKit",         //
    "Kitchen.Sink",         // "ESP32 BattConn",
    "Vranic.SuperMiniBlue", // "ESP32C3 Super Mini (blue, larger ceramic antenna)",
    "Vranic.ESP32C3ant1",   // "ESP32C3 Super Mini (black w/ DIY antenna)",
    "Vranic.ESP32C3Xiao",   // "ESP32-C3 XIAO",
};
