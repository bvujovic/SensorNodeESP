#include <Arduino.h>
#include "bsec.h" // boschsensortec/BSEC Software Library@^1.8.1492
Bsec iaqSensor;
String output = "";

#include "OneButton.h"
const byte pinBtnSave = 4; // Pin for the button to save settings
OneButton btnSave(pinBtnSave, true);

const byte pinLed = 18;

void ledOn(bool state)
{
    digitalWrite(pinLed, state ? HIGH : LOW);
}

void checkIaqSensorStatus()
{
    if (iaqSensor.bsecStatus != BSEC_OK)
    {
        Serial.print("BSEC error code : ");
        Serial.println(iaqSensor.bsecStatus);
    }
    if (iaqSensor.bme68xStatus != BME68X_OK)
    {
        Serial.print("BME680 error code : ");
        Serial.println(iaqSensor.bme68xStatus);
    }
}

#include <EEPROM.h>
#define EEPROM_SIZE 512
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};

void loadState()
{
    if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE)
    {
        Serial.println("Restoring BSEC state...");
        for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
            bsecState[i] = EEPROM.read(i + 1);
        iaqSensor.setState(bsecState);
    }
    else
        Serial.println("No saved state found.");
}

void saveState()
{
    Serial.println("Saving BSEC state...");
    if (iaqSensor.iaqAccuracy >= 2)
    {
        iaqSensor.getState(bsecState);
        EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
        for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
            EEPROM.write(i + 1, bsecState[i]);
        EEPROM.commit();
    }
    else
        Serial.println("IAQ accuracy is too low, not saving state.");
}

void setup()
{
    pinMode(pinLed, OUTPUT);
    ledOn(false);
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    Wire.begin(); // SDA, SCL defaults

    iaqSensor.begin(BME68X_I2C_ADDR_HIGH, Wire);
    checkIaqSensorStatus();

    bsec_virtual_sensor_t sensorList[] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS};

    iaqSensor.updateSubscription(sensorList, sizeof(sensorList) / sizeof(sensorList[0]), BSEC_SAMPLE_RATE_LP);
    checkIaqSensorStatus();
    loadState(); // Load previous state if available

    btnSave.attachClick(
        []()
        {
            saveState();
            for (size_t i = 0; i < 3; i++) // Blink LED to indicate save
            {
                ledOn(true);
                delay(500);
                ledOn(false);
                delay(500);
            }
        });
}

void loop()
{
    btnSave.tick();
    if (iaqSensor.run())
    {
        // Only print when new data is available
        output = String("IAQ: ") + iaqSensor.iaq + " (" + iaqSensor.iaqAccuracy + "), " +
                 "eCO2: " + iaqSensor.co2Equivalent + " ppm, " +
                 "TVOC: " + iaqSensor.breathVocEquivalent + " ppb, " +
                 "Temp: " + iaqSensor.rawTemperature + " °C, " +
                 "Hum: " + iaqSensor.rawHumidity + " %, " +
                 "Pressure: " + (int)(iaqSensor.pressure / 100) + " hPa";
        Serial.println(output);
        Serial.println(String(iaqSensor.temperature) + "\t" + String(iaqSensor.humidity));
        ledOn(iaqSensor.iaqAccuracy >= 2); // Turn on LED if IAQ accuracy is good
    }
    delay(20); // to avoid hammering CPU
}
