const int pinLed = 0;

void setup()
{
  pinMode(pinLed, OUTPUT);
}

void loop()
{
  digitalWrite(pinLed, true);
  delay(500);
  digitalWrite(pinLed, false);
  delay(500);
}
