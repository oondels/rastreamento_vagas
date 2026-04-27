#pragma once

#include "ConfigManager.h"
#include "MqttManager.h"
#include "NetworkManager.h"
#include "ParkingSpotManager.h"

class ParkingController {
 public:
  void begin();
  void update();

 private:
  AppConfig config_;
  ConfigManager configManager_;
  ParkingSpotManager spotManager_;
  WifiNetworkManager networkManager_;
  MqttManager mqttManager_;
  bool initialized_ = false;
  bool publishPending_ = false;
  uint32_t lastPublishMs_ = 0;
};
