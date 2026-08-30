//* Target: Idle single shot mode - 300 sec (5 min) consumes 450µA @ 3.3V
//* it is recommended to adjust the sensor’s standard ASC period parameter depending on
//* the average sampling period achieved during idle single shot operation.
//* Every 5min read CO2 and send it to the hub (SensorNodeESP project)

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h> // sensirion/Sensirion I2C SCD4x@^1.1.0

const byte pinLed = 10;

SensirionI2cScd4x scd;
void takeSingleShotReading()
{
  //* Call this if the sensor was in periodic measurement mode before
  // scd.stopPeriodicMeasurement();

  // some extra time for user to enter serial monitor
  digitalWrite(pinLed, HIGH);
  delay(1000);
  digitalWrite(pinLed, LOW);

  auto error = scd.measureSingleShot();
  if (error)
  {
    Serial.println("Failed to trigger single shot.");
    return;
  }

  // Wait for measurement completion
  bool isDataReady = false;
  ulong startTime = millis();
  const ulong TIMEOUT_MS = 6000; // SCD41 takes

  // Poll until data is ready or timeout expires
  while (!isDataReady && (millis() < startTime + TIMEOUT_MS))
  {
    delay(100); // yield to avoid hammering the I2C bus
    scd.getDataReadyStatus(isDataReady);
  }
  Serial.printf("Single-Shot Measurement Time: %lu ms\n", millis() - startTime);

  if (isDataReady)
  {
    uint16_t co2 = 0;
    float temp = 0.0f, humidity = 0.0f;
    error = scd.readMeasurement(co2, temp, humidity);
    if (!error)
      Serial.printf("CO2: %d ppm | T: %.2f °C | RH: %.2f %%\n", co2, temp, humidity);
    delay(1000);
  }
  else
    Serial.println("Data not ready yet.");
}

void setup()
{
  pinMode(pinLed, OUTPUT);
  Serial.begin(115200);
  while (!Serial)
    delay(100);

  Wire.begin();          // Adjust SDA/SCL pins here if needed (e.g., Wire.begin(21, 22))
  scd.begin(Wire, 0x62); // Use the default I2C address for SCD41 (0x62)

  //* Power Cycled Single Shot Operation: use wakeUp() and powerDown() to save power between readings
  //* ASC is not available in that mode
  // scd.wakeUp();
  takeSingleShotReading();
  // scd.powerDown();

  float tempOffset = 0.0f;
  // scd.setTemperatureOffset(tempOffset);
  auto error = scd.getTemperatureOffset(tempOffset);
  if (!error)
    printf("Temperature offset set to %.2f °C\n", tempOffset);
  else
    printf("Failed to get temperature offset, error code: %d\n", error);

  uint16_t alt = 170;
  // scd.setSensorAltitude(alt);
  error = scd.getSensorAltitude(alt);
  if (!error)
    printf("Altitude: %d m\n", alt);
  else
    printf("Failed to get altitude, error code: %d\n", error);

  //* Persist settings to EEPROM (optional, only needed if you want to save the current configuration)
  // scd.persistSettings();
  // Serial.println("Settings persisted to EEPROM.");

  Serial.println("Entering MCU sleep cycle...");
  esp_sleep_enable_timer_wakeup(2 * 60 * 1000000);
  esp_deep_sleep_start();
}

void loop() { delay(100); }
