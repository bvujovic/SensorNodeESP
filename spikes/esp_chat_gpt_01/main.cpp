#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

const int pirPin = D2;  // Replace D2 with the GPIO pin connected to the PIR sensor
const int ledPin = LED_BUILTIN;  // Replace D1 with the GPIO pin connected to an LED for testing

volatile bool motionDetectedFlag = false;

void IRAM_ATTR handleInterrupt() {
  motionDetectedFlag = true;
}

void setup() {
  pinMode(pirPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  Serial.println("Device is starting...");

  // Attach the interrupt to the PIR sensor pin
  attachInterrupt((pirPin), handleInterrupt, CHANGE);

  // Add any additional setup code here if needed
}

void loop() {
  if (motionDetectedFlag) {
    // Do something when motion is detected
    Serial.println("Motion detected!");
    digitalWrite(ledPin, HIGH);  // Turn on an LED for testing
    delay(1000);  // Add your code to perform some action

    // Add any additional code for your specific task here

    digitalWrite(ledPin, LOW);  // Turn off the LED

    // Reset the flag
    motionDetectedFlag = false;
  }

  // Go to sleep indefinitely until an interrupt wakes it up
  Serial.println("Going to sleep...");
  ESP.deepSleep(0);  // Sleep indefinitely until an interrupt occurs
}
