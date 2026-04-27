#include "MqttManager.h"

#include <ArduinoJson.h>

void MqttManager::begin(const AppConfig& config, uint32_t reconnectIntervalMs) {
  mqttConfig_ = config.mqtt;
  deviceConfig_ = config.device;
  reconnectIntervalMs_ = reconnectIntervalMs;
  mqttClient_.setBufferSize(MQTT_PACKET_BUFFER_SIZE);
  mqttClient_.setServer(mqttConfig_.host.c_str(), mqttConfig_.port);
}

void MqttManager::update(uint32_t nowMs, bool wifiConnected) {
  if (!wifiConnected) {
    return;
  }

  if (mqttClient_.connected()) {
    mqttClient_.loop();
    return;
  }

  if (hasAttemptedConnection_ &&
      nowMs - lastAttemptMs_ < reconnectIntervalMs_) {
    return;
  }

  const String availabilityTopic = "estacionamento/" + deviceConfig_.siteId +
                                   "/" + deviceConfig_.deviceId +
                                   "/availability";

  Serial.print(F("[MQTT] conectando em "));
  Serial.print(mqttConfig_.host);
  Serial.print(F(":"));
  Serial.println(mqttConfig_.port);

  const bool connected = mqttClient_.connect(
      mqttConfig_.clientId.c_str(), mqttConfig_.username.c_str(),
      mqttConfig_.password.c_str(), availabilityTopic.c_str(), 0, true,
      "offline");

  lastAttemptMs_ = nowMs;
  hasAttemptedConnection_ = true;

  if (!connected) {
    Serial.print(F("[MQTT] falha na conexao, rc="));
    Serial.println(mqttClient_.state());
    return;
  }

  mqttClient_.publish(availabilityTopic.c_str(), "online", true);
  Serial.println(F("[MQTT] conectado"));
  reconnected_ = true;
}

bool MqttManager::isConnected() { return mqttClient_.connected(); }

bool MqttManager::consumeReconnectFlag() {
  const bool reconnected = reconnected_;
  reconnected_ = false;
  return reconnected;
}

bool MqttManager::publishStatus(const AppConfig& config,
                                const ParkingSpotManager& spotManager,
                                uint32_t nowMs) {
  if (!mqttClient_.connected()) {
    return false;
  }

  DynamicJsonDocument doc(512 + spotManager.getSpotCount() * 96U);
  doc["siteId"] = config.device.siteId;
  doc["deviceId"] = config.device.deviceId;
  doc["timestampMs"] = nowMs;

  JsonArray spotsArray = doc["spots"].to<JsonArray>();
  for (size_t index = 0; index < spotManager.getSpotCount(); ++index) {
    const ParkingSpot& spot = spotManager.getSpot(index);
    JsonObject spotJson = spotsArray.add<JsonObject>();
    spotJson["id"] = spot.getId();
    spotJson["occupied"] = spot.isOccupied();
    spotJson["pin"] = spot.getPin();
  }

  String payload;
  serializeJson(doc, payload);

  const String statusTopic = buildStatusTopic(config);
  const bool published = mqttClient_.publish(
      statusTopic.c_str(), payload.c_str(), config.mqtt.publishRetained);

  if (published) {
    Serial.println(F("[MQTT] status publicado"));
  } else {
    Serial.println(F("[MQTT] falha ao publicar status"));
    Serial.print(F("[MQTT] tamanho do payload: "));
    Serial.println(payload.length());
    Serial.print(F("[MQTT] tamanho do topico: "));
    Serial.println(statusTopic.length());
  }

  return published;
}

String MqttManager::buildStatusTopic(const AppConfig& config) const {
  return "estacionamento/" + config.device.siteId + "/" +
         config.device.deviceId + "/status";
}

String MqttManager::buildAvailabilityTopic(const AppConfig& config) const {
  return "estacionamento/" + config.device.siteId + "/" +
         config.device.deviceId + "/availability";
}
