#pragma once

#include <stdint.h>

/// @brief
struct AirData
{
    /// @brief Temperature in Celzius.
    int8_t temperature;
    /// @brief Relative humidity.
    uint8_t humidity;
    /// @brief 0-Normal operation, 1-Warm-Up phase, first 3 minutes after power-on.
    /// 2-Initial Start-Up phase, first full hour of operation after initial power-on.
    uint8_t status;
    /// @brief Air quality index: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy.
    uint8_t AQI;
    /// @brief Total volatile organic compounds (ppb). Range: 0–65000; <400 good, >400 bad.
    uint16_t TVOC;
    /// @brief CO2 equivalent (ppm). Range: 400–65000.
    /// Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000), Poor(1000 - 1500), Unhealthy(> 1500).
    uint16_t ECO2;
};
