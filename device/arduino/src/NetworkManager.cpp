#include "NetworkManager.h"

void WifiNetworkManager::begin(const WifiConfig& wifiConfig,
                               uint32_t reconnectIntervalMs) {
  wifiConfig_ = wifiConfig;
  reconnectIntervalMs_ = reconnectIntervalMs;
  WiFi.mode(WIFI_STA);
}

void WifiNetworkManager::update(uint32_t nowMs) {
  if (isConnected()) {
    return;
  }

  if (hasAttemptedConnection_ &&
      nowMs - lastAttemptMs_ < reconnectIntervalMs_) {
    return;
  }

  Serial.print(F("[WiFi] conectando em "));
  Serial.println(wifiConfig_.ssid);
  WiFi.begin(wifiConfig_.ssid.c_str(), wifiConfig_.password.c_str());
  lastAttemptMs_ = nowMs;
  hasAttemptedConnection_ = true;
}

bool WifiNetworkManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}
