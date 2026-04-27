#pragma once

#include <WiFi.h>

#include "AppConfig.h"

class WifiNetworkManager {
 public:
  void begin(const WifiConfig& wifiConfig, uint32_t reconnectIntervalMs);
  void update(uint32_t nowMs);
  bool isConnected() const;

 private:
  WifiConfig wifiConfig_;
  uint32_t reconnectIntervalMs_ = 10000;
  uint32_t lastAttemptMs_ = 0;
  bool hasAttemptedConnection_ = false;
};
