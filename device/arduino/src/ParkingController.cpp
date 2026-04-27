#include "ParkingController.h"

void ParkingController::begin() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("[Boot] iniciando controlador"));

  if (!configManager_.beginAndLoad(config_)) {
    Serial.println(F("[Boot] inicializacao abortada por erro de configuracao"));
    initialized_ = false;
    return;
  }

  const uint32_t nowMs = millis();
  spotManager_.begin(config_, nowMs);
  networkManager_.begin(config_.wifi, config_.timing.wifiReconnectIntervalMs);
  mqttManager_.begin(config_, config_.timing.mqttReconnectIntervalMs);

  publishPending_ = true;
  lastPublishMs_ = nowMs;
  initialized_ = true;
}

void ParkingController::update() {
  if (!initialized_) {
    return;
  }

  const uint32_t nowMs = millis();
  networkManager_.update(nowMs);
  mqttManager_.update(nowMs, networkManager_.isConnected());

  if (mqttManager_.consumeReconnectFlag()) {
    publishPending_ = true;
  }

  if (spotManager_.update(nowMs)) {
    publishPending_ = true;
  }

  if (config_.timing.publishIntervalMs > 0 &&
      nowMs - lastPublishMs_ >= config_.timing.publishIntervalMs) {
    publishPending_ = true;
  }

  if (!publishPending_ || !mqttManager_.isConnected()) {
    return;
  }

  if (mqttManager_.publishStatus(config_, spotManager_, nowMs)) {
    publishPending_ = false;
    lastPublishMs_ = nowMs;
  }
}
