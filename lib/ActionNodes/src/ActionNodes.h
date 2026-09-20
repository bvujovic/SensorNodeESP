#pragma once

#include <WiFiClientSecure.h>
#include <PubSubClient.h> // lib_deps = knolleary/PubSubClient @ ^2.8
#include <azure-secrets.h>
#include "Logger.h"

class ActionNodes
{
private:
  WiFiClientSecure azureSecureClient;
  // PubSubClient azureMqttClient(azureSecureClient);
  PubSubClient *azureMqttClient;

  // Remote Node Wake Queue Variables
  bool hasPendingCommand = false;
  uint8_t pendingCommandPayload = 0;

  Logger &logger;

public:
  ActionNodes(Logger &logger);

  void init();
  void loop();

  // Manage connecting/reconnecting to Azure
  void connectToAzure();
  // Handle incoming commands sent from the Azure Cloud
  void azureCallback(char *topic, byte *payload, unsigned int length);
};
