#pragma once

#include <Arduino.h>

constexpr size_t MAX_SPOTS = 16;

struct WifiConfig {
  String ssid;
  String password;
};

struct MqttConfig {
  String host;
  uint16_t port = 1883;
  String username;
  String password;
  String clientId;
  bool publishRetained = true;
};

struct DeviceConfig {
  String siteId;
  String deviceId;
};

struct TimingConfig {
  uint32_t sensorDebounceMs = 120;
  uint32_t wifiReconnectIntervalMs = 10000;
  uint32_t mqttReconnectIntervalMs = 5000;
  uint32_t publishIntervalMs = 0;
};

struct ParkingSpotDefinition {
  String id;
  uint8_t pin = 0;
  bool occupiedOnHigh = true;
};

struct AppConfig {
  WifiConfig wifi;
  MqttConfig mqtt;
  DeviceConfig device;
  TimingConfig timing;
  ParkingSpotDefinition spots[MAX_SPOTS];
  size_t spotCount = 0;
};
