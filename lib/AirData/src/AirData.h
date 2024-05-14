#pragma once

typedef unsigned short ushort;

/// @brief Air quality Data from ENS160 sensor.
class AirData
{
private:
    /// @brief Air quality index: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy.
    ushort AQI;
    /// @brief Total volatile organic compounds (ppb). Range: 0–65000; <400 good, >400 bad.
    ushort TVOC;
    /// @brief CO2 equivalent (ppm). Range: 400–65000.
    /// Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000), Poor(1000 - 1500), Unhealthy(> 1500).
    ushort ECO2;

    // TODO Dodati vreme kada su ove vrednosti izmerene

public:
    AirData(ushort aqi, ushort tvoc, ushort eco2);

    ushort getAQI() { return AQI; }
    ushort getTVOC() { return TVOC; }
    ushort getECO2() { return ECO2; }
};
