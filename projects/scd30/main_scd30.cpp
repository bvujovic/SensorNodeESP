/*
ESP8266, SCD30, TM1637, taster
Display shows CO2 concentration, temperature and humidity.
Press button to change display mode: CO2 or temp:hum.
Press and hold button to toggle measurement interval between 5 and 60 seconds.

ChatGPT on calibration (conversation "SCD30 CO2 Sensor Review"):
You can disable ASC and apply manual forced calibration once, like this:
After 20 minutes of stable outdoor placement:
airSensor.forceRecalibrationWithReference(400);
airSensor.setAutoSelfCalibration(false); // Optional: turn ASC off if you don't ventilate regularly
This tells the sensor, “You are now in 400 ppm air—set this as your reference.”
*/

#include "Arduino.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" // sparkfun/SparkFun SCD30 Arduino Library@^1.0.20
SCD30 airSensor;

// enum DisplayMode
// {
//     // Interval, // Display measurement interval
//     CO2 = 0, // Display CO2 concentration
//     TempHum, // Display temperature and humidity
//     MaxModes // Number of display modes
// };
// DisplayMode displayMode = CO2;         // Start with CO2 display mode
int prevCO2 = 0;                       // Previous CO2 value
float prevTemp = 0.0f, prevHum = 0.0f; // Previous temperature and humidity values

#include "OneButton.h"         // mathertel/OneButton@^2.0.0
const byte pinButton = D6;
OneButton btn(pinButton, true); // Button to change display mode

// // Show temp:hum on the display
// void displayTempHum(float temp, float hum)
// {
//     int t = temp + 0.5; // Round temperature to nearest integer
//     int h = hum + 0.5;  // Round humidity to nearest integer
//     display.showNumberDecEx(t * 100 + h, 0b01000000, true);
// }

// void displayValue(uint16_t co2, float temp, float hum)
// {
//     if (displayMode == CO2)
//         display.showNumberDec(co2 < 9999 ? co2 : 9999); // Show CO2 on the display, max 9999
//     else if (displayMode == TempHum)
//         displayTempHum(temp, hum);
// }

void displayValue(uint16_t co2, float temp, float hum)
{
    Serial.print("Displaying CO2: ");
    Serial.print(co2);
    Serial.print(" ppm, Temp: ");
    Serial.print(temp, 1);
    Serial.print(" C, Humidity: ");
    Serial.print(hum, 1);
    Serial.println(" %");
}

ulong msStartFrcCD = 0;

void setup()
{
    pinMode(pinButton, INPUT_PULLUP);
    // display.setBrightness(2);
    // display.clear();
    Serial.begin(115200);
    Wire.begin();
    if (airSensor.begin() == false)
    {
        Serial.println("Air sensor not detected. Please check wiring. Freezing...");
        // display.showNumberDec(-1); // Show -1 on the display to indicate error
        while (1)
            delay(100); // Do nothing forevermore
    }
    // display.showNumberDec(0);               // Show 0 on the display to indicate sensor is ready
    airSensor.setAutoSelfCalibration(true); // Enable auto self calibration
    Serial.print("Auto calibration set to: ");
    Serial.println(airSensor.getAutoSelfCalibration());
    // airSensor.setForcedRecalibrationFactor(400); // Set forced recalibration factor to 400 ppm

    airSensor.setAltitudeCompensation(170); // Set altitude compensation to 170m
    Serial.print("Altitude compensation set to: ");
    Serial.println(airSensor.getAltitudeCompensation());
    airSensor.setMeasurementInterval(5);
    Serial.print("Measurement interval set to: ");
    Serial.println(airSensor.getMeasurementInterval());

    // btn.attachClick(
    //     []()
    //     {
    //         Serial.println("Button clicked, changing display mode...");
    //         displayMode = (DisplayMode)((int(displayMode) + 1) % int(MaxModes)); // Cycle through display modes
    //         displayValue(prevCO2, prevTemp, prevHum);                            // Show previous CO2, temperature and humidity
    //     });
    btn.attachLongPressStart(
        []()
        {
            Serial.println("Button long pressed, toggling measurement interval...");
            airSensor.setMeasurementInterval(airSensor.getMeasurementInterval() == 60 ? 5 : 60); // Toggle measurement interval
            // display.showNumberDec(airSensor.getMeasurementInterval());                           // Show measurement interval on the display
            Serial.print("Measurement interval set to: ");
            Serial.println(airSensor.getMeasurementInterval());
            delay(1000);                              // Wait for a second to let the user read the display
            displayValue(prevCO2, prevTemp, prevHum); // Show previous CO2, temperature and humidity
        });
    btn.attachDoubleClick(
        []()
        {
            // delay(60000); // Wait for 60 seconds to let the sensor stabilize
            msStartFrcCD = millis();                      // Start time for forced calibration
            Serial.println("Button double clicked, starting forced recalibration with 425 ppm reference in 20 minutes.");
            // display.showNumberDecEx(0, 0b01000000, true); // Show 00:00 on the display to indicate forced recalibration
            // airSensor.setForcedRecalibrationFactor(425); // Set forced recalibration factor to 425 ppm
            // airSensor.setAutoSelfCalibration(false); // Optional: turn ASC off if you don't ventilate regularly
            // Serial.println("Button double clicked, forced recalibration with 400 ppm reference.");
            // display.showNumberDec(425); // Show 425 on the display to indicate forced recalibration
            delay(1000); // Wait for a second to let the user read the display
            airSensor.setMeasurementInterval(60);
        });
}

ulong ms = 0; // Variable to store the last time the sensor was checked

void loop()
{
    if (millis() > ms + 1000)
    {
        if (airSensor.dataAvailable())
        {
            uint16_t co2 = airSensor.getCO2();
            float temp = airSensor.getTemperature();
            float hum = airSensor.getHumidity();
            Serial.print("CO2(ppm):");
            Serial.print(co2);
            Serial.print(" temp(C):");
            Serial.print(temp, 1);
            Serial.print(" humidity(%):");
            Serial.print(hum, 1);
            Serial.println();
            if (msStartFrcCD > 0)
            {
                Serial.println("Forced recalibration in progress...");
                // display.showNumberDec(1111);
                delay(1000); // Show 1111 on the display to indicate forced recalibration in progress
            }
            displayValue(co2, temp, hum);
            prevCO2 = co2;
            prevTemp = temp;
            prevHum = hum;
        }
        ms = millis();
    }

    if (msStartFrcCD > 0 && millis() >= msStartFrcCD + 20UL * 60 * 1000) // 20 minutes passed
    {
        msStartFrcCD = 0;                            // Reset the start time
        airSensor.setForcedRecalibrationFactor(425); // Set forced recalibration factor to 425 ppm
        airSensor.setAutoSelfCalibration(false);     // Optional: turn ASC off if you don't ventilate regularly
        Serial.println("Forced recalibration with 425 ppm reference completed.");
        // display.showNumberDec(425); // Show 425 on the display to indicate forced recalibration
        delay(1000);                // Wait for a second to let the user read the display
    }

    btn.tick();
    delay(10);
}
