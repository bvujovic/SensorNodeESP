#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h>

SensirionI2cScd4x scd;

const ulong REPORT_INTERVAL_MS = 1 * 60 * 1000UL;
ulong lastReportTime = 0;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(100);

    // Wire.begin();          // Adjust pin arguments here if non-standard SDA/SCL are used (e.g., Wire.begin(21, 22))
    // Wire.setClock(100000); // 100 kHz standard I2C speed
    // scd.begin(Wire);
    Wire.begin(8, 9);
    scd.begin(Wire, 0x62); // Use the default I2C address for SCD41 (0x62)
    scd.stopPeriodicMeasurement();
    delay(500);
    auto error = scd.startPeriodicMeasurement(); // sensor updates internally every 5 seconds
    if (error)
        Serial.println("Error: Failed to start SCD41 periodic measurements!");
    else
        Serial.println("SCD41 Node Initialized. Reporting CO2 every 10 minutes...");
    lastReportTime = 0;
}

void loop()
{
    ulong currentMillis = millis();

    // Check if 10 minutes have elapsed
    if (currentMillis - lastReportTime >= REPORT_INTERVAL_MS)
    {
        lastReportTime = currentMillis;

        bool isDataReady = false;
        // scd.getDataReadyFlag(isDataReady);
        ulong startTime = millis();
        const ulong TIMEOUT_MS = 3000;

        // Poll until data is ready or timeout expires
        while (!isDataReady && (millis() < startTime + TIMEOUT_MS))
        {
            delay(100); // yield to avoid hammering the I2C bus
            scd.getDataReadyStatus(isDataReady);
        }

        if (isDataReady)
        {
            uint16_t co2 = 0;
            float temperature = 0.0f;
            float humidity = 0.0f;

            uint16_t error = scd.readMeasurement(co2, temperature, humidity);
            if (!error)
            {
                Serial.println("--- [Node Report] ---");
                Serial.printf("CO2: %u ppm\n", co2);
                Serial.printf("Temperature: %.2f °C\n", temperature);
                Serial.printf("Humidity: %.2f %%\n", humidity);
                Serial.println("---------------------");

                // === PLACE YOUR ESP-NOW TRANSMISSION LOGIC HERE ===
            }
            else
            {
                Serial.println("Error: Failed to read measurement data from SCD41.");
            }
        }
        else
        {
            Serial.println("Warning: Measurement data not ready at 10-minute interval.");
        }
    }
    delay(20);
}
