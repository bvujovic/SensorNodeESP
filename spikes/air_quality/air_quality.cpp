//* lib_deps = dfrobot/DFRobot_ENS160@^1.0.1

#include <DFRobot_ENS160.h>

//* https://www.instructables.com/ENS160-AHT21-Sensor-for-Arduino/

#define I2C_COMMUNICATION // I2C communication. Comment out this line of code if you want to use SPI communication.

#ifdef I2C_COMMUNICATION
/**
 *   For Fermion version, the default I2C address is 0x53, connect SDO pin to GND and I2C address will be 0x52
 */
DFRobot_ENS160_I2C ens(&Wire, /*I2CAddr*/ 0x53);
#else
/**
 * Set up digital pin according to the on-board pin connected with SPI chip-select pin
 * csPin Available Pins. For example: ESP32&ESP8266(D3), m0(6)
 */
uint8_t csPin = D3;
DFRobot_ENS160_SPI ens(&SPI, csPin);
#endif

const byte pinDisplejCLK = D6;
const byte pinDisplejDIO = D5;
#include <TM1637Display.h>
TM1637Display disp(pinDisplejCLK, pinDisplejDIO);

#include "AirData.h"

#include "ESP8266WiFi.h"
#include "CredWiFi.h"
#include <SNTPtime.h>

SNTPtime ntp;
StrDateTime now;

void wiFiOff()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
}

bool getCurrentTime()
{
    const ulong maxTrySetTime = 3;
    ulong cntTrySetTime = 0;
    while (!ntp.setSNTPtime() && cntTrySetTime++ < maxTrySetTime)
        ;
    return cntTrySetTime < maxTrySetTime;
}

void initTime()
{
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
        delay(250);
    if (WiFi.status() == WL_CONNECTED)
    {
        getCurrentTime();
        wiFiOff();
    }
}

#include <LittleFS.h>

void logData(AirData &ad)
{
    File f = LittleFS.open("air_quality.csv", "a");
    if (f)
    {
        Serial.printf("%04d-%02d-%02d ", now.year, now.month, now.day);
        Serial.printf("%02d:%02d:%02d\n", now.hour, now.minute, now.second);
        String s;
        s = String(now.year) + "-" + now.month + "-" + now.day + " " + now.hour + ":" + now.minute;
        s += String("\t") + ad.getAQI() + "\t" + ad.getTVOC() + "\t" + ad.getECO2() + "\n";
        f.write(s.c_str());
        f.close();
    }
}

// Milliseconds in 1 second.
#define SEC (1000)
// Milliseconds in 1 minute.
#define MIN (60 * SEC)

enum Displayed
{
    AQI,
    TVOC,
    ECO2,
    Nothing
} displayed;
ulong msLastDisplay;

byte loggedMinute = 123;

void setup(void)
{
    Serial.begin(115200);

    initTime();
    now = ntp.getTime(1.0, 1);
    Serial.println(now.minute);

    disp.setBrightness(1);
    displayed = AQI;
    msLastDisplay = millis();

    // Init the sensor
    while (NO_ERR != ens.begin())
    {
        Serial.println("Communication with device failed, please check connection");
        delay(3000);
    }
    Serial.println("Begin ok!");

    ens.setPWRMode(ENS160_STANDARD_MODE);
    ens.setTempAndHum(/*temperature=*/23.0, /*humidity=*/50.0);
}

void loop()
{
    // uint8_t Status = ens.getENS160Status();
    // Serial.print("Sensor operating status : ");
    // Serial.println(Status);

    AirData ad(ens.getAQI(), ens.getTVOC(), ens.getECO2());
    // Serial.printf("Air quality index : %u\n", ad.getAQI());
    // Serial.printf("Concentration of total volatile organic compounds: %u ppb\n", ad.getTVOC());
    // Serial.printf("Carbon dioxide equivalent concentration: %d ppm\n", ad.getECO2());

    ulong ms = millis();
    bool somethingDisplayed = (displayed == AQI || displayed == TVOC || displayed == ECO2) && ms > msLastDisplay + SEC;
    bool nothingDisplayed = (displayed == Nothing) && ms > msLastDisplay + 7 * SEC;
    if (somethingDisplayed || nothingDisplayed)
    {
        if (somethingDisplayed)
            displayed = Displayed(displayed + 1);
        else
            displayed = AQI;
        msLastDisplay = ms;

        if (displayed != Nothing)
        {
            ushort x = displayed == AQI ? ad.getAQI() : displayed == TVOC ? ad.getTVOC()
                                                                          : ad.getECO2();
            disp.showNumberDec(x);
        }
        else
            disp.clear();
    }

    now = ntp.getTime(1.0, 1);
    if (now.second == 0 && now.minute != loggedMinute)
    {
        loggedMinute = now.minute;
        logData(ad);
    }
}
