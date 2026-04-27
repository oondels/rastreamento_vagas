#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "AppConfig.h"
#include "ParkingSpotManager.h"

class MqttManager {
 public:
  void begin(const AppConfig& config, uint32_t reconnectIntervalMs);
  void update(uint32_t nowMs, bool wifiConnected);
  bool isConnected();
  bool consumeReconnectFlag();
  bool publishStatus(const AppConfig& config,
                     const ParkingSpotManager& spotManager, uint32_t nowMs);

 private:
  String buildStatusTopic(const AppConfig& config) const;
  String buildAvailabilityTopic(const AppConfig& config) const;

  WiFiClient wifiClient_;
  PubSubClient mqttClient_{wifiClient_};
  MqttConfig mqttConfig_;
  DeviceConfig deviceConfig_;
  uint32_t reconnectIntervalMs_ = 5000;
  uint32_t lastAttemptMs_ = 0;
  bool hasAttemptedConnection_ = false;
  bool reconnected_ = false;
};
