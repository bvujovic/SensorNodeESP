#include "ActionNodes.h"

ActionNodes::ActionNodes(Logger &logger)
    : logger(logger)
{
  // azureMqttClient = new PubSubClient(azureSecureClient);
}

void ActionNodes::init()
{
  // Azure IoT Hub
  azureSecureClient.setInsecure(); // Don't validate the TLS certificate chain (saves memory and avoids certificate expiration crashes)
  azureMqttClient = new PubSubClient(azureSecureClient);
  azureMqttClient->setServer(SECRET_MQTT_SERVER, 8883);

  // azureMqttClient->setCallback(azureCallback);
  azureMqttClient->setCallback(
      [this](char *topic, byte *payload, unsigned int length)
      {
        azureCallback(topic, payload, length);
      });
}

void ActionNodes::loop()
{
  Serial.print(azureMqttClient->connected());
  if (!azureMqttClient->connected())
  {
    static ulong lastReconnectAttempt = 0;
    ulong now = millis();
    if (now - lastReconnectAttempt > 10000) //* make this interval longer after a few tries
    {
      lastReconnectAttempt = now;
      connectToAzure();
    }
  }
  else
  {
    azureMqttClient->loop();
    // Serial.print('l');
  }
}

void ActionNodes::connectToAzure()
{
  if (azureMqttClient->connected())
    return;

  Serial.print("Attempting Azure IoT Hub connection... ");

  // Azure requires the ClientID, Username, and SAS Token Password
  if (azureMqttClient->connect(SECRET_DEVICE_ID, SECRET_MQTT_USER, SECRET_SAS_TOKEN))
  {
    Serial.println("Connected to Azure!");
    auto res = azureMqttClient->subscribe("devices/TestSensorHub/messages/devicebound/#");
    Serial.printf("Subscribed to Azure topic: %s, result: %d\n", "devices/TestSensorHub/messages/devicebound/#", res);
  }
  else
    Serial.printf("Failed connection, rc=%d. Try again in next loop.\n", azureMqttClient->state());
}

void ActionNodes::azureCallback(char *topic, byte *payload, unsigned int length)
{
  String s = "azureCallback";
  Serial.print("Cloud message arrived on topic: ");
  Serial.println(topic);
  if (length > 0)
  {
    // Only for testing! Capture the command byte (e.g. '1', '2', etc.)
    pendingCommandPayload = payload[0];
    hasPendingCommand = true;
    Serial.printf("Command buffered for battery node: %c\n", pendingCommandPayload);
    s += String(" - Command buffered for battery node: ") + (char)pendingCommandPayload;
    logger.add("Azure", "HUB", s.c_str());
  }
}
