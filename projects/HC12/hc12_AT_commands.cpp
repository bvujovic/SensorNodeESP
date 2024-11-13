//* https://www.datsi.fi.upm.es/docencia/DMC/HC-12_v2.3A.pdf
//* https://wolles-elektronikkiste.de/en/hc-12-radio-module
//* https://www.esp8266.com/viewtopic.php?p=64663

#include <Arduino.h>

const byte pinSet = 0;
//? auto hc = Serial2;

void setup()
{
  pinMode(pinSet, OUTPUT);
  // digitalWrite(pinSet, HIGH); // For data transmission
  digitalWrite(pinSet, LOW); // For AT commands, e.g. AT+RX
  Serial2.begin(9600);       // TX2, RX2: UART connection to HC-12
  Serial.begin(115200);
  Serial.println("\n *** HC12 Start ***");
  Serial2.println("AT+RX");
  delay(100);
}

void loop()
{
  while (Serial2.available())
  {
    Serial.println(Serial2.readString());
  }
  while (Serial.available())
  {
    Serial2.write(Serial.read());
  }
}
