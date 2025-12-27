// Modified for ESP32-C3 + BSEC2

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

// Include BSEC2 library instead of BSEC v1.x
#include "bsec2.h"

// If needed, define macros to select RISC-V build, etc.
// #define BSEC2_USE_RISCV  // (depending on how the library is packaged)

Bsec2 bsec;

String output = "";

#include "AirData.h"
AirData airData;

#define SECOND (1000UL)
#define MINUTE (60UL * SECOND)

#include "OneButton.h" // for button handling
const byte pinBtnSave = 4;
OneButton btnSave(pinBtnSave, true);

const byte pinLed = 18;

void ledOn(bool state)
{
  digitalWrite(pinLed, state ? HIGH : LOW);
}

void ledOnDelay(int secs)
{
  ledOn(true);
  delay(secs * SECOND);
  ledOn(false);
}

// Constants for I²C address
#define BME68X_I2C_ADDR_LOW 0x76
#define BME68X_I2C_ADDR_HIGH 0x77

// EEPROM size & state buffer (size may differ in BSEC2)
#define EEPROM_SIZE 1024
static uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};

void checkIaqSensorStatus()
{
  // Serial.println("checkIaqSensorStatus");
  // if (iaqSensor.status != BSEC_OK) {
  //   Serial.print("BSEC2 error code: ");
  //   Serial.println(iaqSensor.status);
  // }
  // if (iaqSensor.sensor.status != BME68X_OK) {
  //   Serial.print("BME68X error code: ");
  //   Serial.println(iaqSensor.sensor.status);
  //   Serial.println(iaqSensor.sensor.statusString());
  // }
  if (bsec.status < BSEC_OK)
  {
    Serial.println("BSEC error code : " + String(bsec.status));
    // errLeds(); /* Halt in case of failure */
  }
  else if (bsec.status > BSEC_OK)
    Serial.println("BSEC warning code : " + String(bsec.status));

  if (bsec.sensor.status < BME68X_OK)
  {
    Serial.println("BME68X error code : " + String(bsec.sensor.status));
    // errLeds(); /* Halt in case of failure */
  }
  else if (bsec.sensor.status > BME68X_OK)
    Serial.println("BME68X warning code : " + String(bsec.sensor.status));
}

void loadState()
{
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE)
  {
    Serial.println("Restoring BSEC2 state...");
    for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
    {
      bsecState[i] = EEPROM.read(i + 1);
    }
    bsec.setState(bsecState);
  }
  else
  {
    Serial.println("No saved state found.");
  }
}

void saveState()
{
  Serial.println("Saving BSEC2 state...");
  if (bsec.iaqAccuracy >= 2)
  {
    iaqSensor.getState(bsecState);
    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
    {
      EEPROM.write(i + 1, bsecState[i]);
    }
    EEPROM.commit();
  }
  else
  {
    Serial.println("IAQ accuracy too low, not saving state.");
  }
}

ulong msLastData = 0;
const ulong itvSendData = 10 * MINUTE - 30 * SECOND; // 10 min interval minus margin
bool newDataAvailable = false;

void setup()
{
  pinMode(pinLed, OUTPUT);
  ledOn(false);
  Serial.begin(115200);
  Wire.begin(); // default SDA,SCL on ESP32-C3 or set explicitly if required

  // Initialise BME680/BSEC2
  if (!bsec.begin(BME68X_I2C_ADDR_HIGH, Wire))
  {
    Serial.println("BME680 not found at address 0x77!");
    while (1)
    {
      delay(1000);
    }
  }
  checkIaqSensorStatus();

  // Subscribe to sensors in BSEC2 (the enumeration may differ slightly)
  bsec_virtual_sensor_t sensorList[] = {
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_STABILIZATION_STATUS,
      BSEC_OUTPUT_RUN_IN_STATUS};
  size_t sensorCount = sizeof(sensorList) / sizeof(sensorList[0]);
  bsec.updateSubscription(sensorList, sensorCount, BSEC_SAMPLE_RATE_LP);

  checkIaqSensorStatus();

  loadState();

  btnSave.attachClick([]()
                      {
    saveState();
    for (size_t i = 0; i < 3; i++) {
      ledOn(true);
      delay(500);
      ledOn(false);
      delay(500);
    } });
}

void loop()
{
  btnSave.tick();

  if (bsec.run())
  {
    newDataAvailable = true;
    // Access the updated data via the new API members
    output = String("IAQ: ") + bsec.iaq + " (" + bsec.iaqAccuracy + "), " +
             "eCO2: " + bsec.co2Equivalent + " ppm, " +
             "TVOC: " + bsec.breathVocEquivalent + " ppb, " +
             "Temp: " + bsec.temperature + " °C, " +
             "Hum: " + bsec.humidity + " %, " +
             "Pres: " + (int)(bsec.pressure / 100) + " hPa";
    Serial.println(output);
  }

  if (newDataAvailable && (msLastData == 0 || millis() > msLastData + itvSendData))
  {
    ledOn(bsec.iaqAccuracy >= 2);
    msLastData = millis();
    newDataAvailable = false;
  }

  delay(20);
}
