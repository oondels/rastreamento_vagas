#include "ConfigManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

bool ConfigManager::beginAndLoad(AppConfig& config) {
  if (!LittleFS.begin(false)) {
    printConfigError(F("falha ao montar LittleFS"));
    return false;
  }

  return loadConfigFromFile(config);
}

bool ConfigManager::loadConfigFromFile(AppConfig& config) {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    printConfigError(F("arquivo /config.json nao encontrado"));
    return false;
  }

  const size_t capacity = static_cast<size_t>(file.size()) * 2U + 2048U;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print(F("[Config] JSON invalido: "));
    Serial.println(error.c_str());
    return false;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  JsonObjectConst wifi = root["wifi"];
  JsonObjectConst mqtt = root["mqtt"];
  JsonObjectConst device = root["device"];
  JsonObjectConst timing = root["timing"];
  JsonArrayConst spots = root["spots"];

  if (wifi.isNull() || mqtt.isNull() || device.isNull() || timing.isNull()) {
    printConfigError(F("secoes obrigatorias ausentes"));
    return false;
  }

  if (spots.isNull()) {
    printConfigError(F("array spots ausente"));
    return false;
  }

  config.wifi.ssid = wifi["ssid"] | "";
  config.wifi.password = wifi["password"] | "";
  config.mqtt.host = mqtt["host"] | "";
  config.mqtt.port = mqtt["port"] | 1883;
  config.mqtt.username = mqtt["username"] | "";
  config.mqtt.password = mqtt["password"] | "";
  config.mqtt.clientId = mqtt["clientId"] | "";
  config.mqtt.publishRetained = mqtt["publishRetained"] | true;
  config.device.siteId = device["siteId"] | "";
  config.device.deviceId = device["deviceId"] | "";
  config.timing.sensorDebounceMs = timing["sensorDebounceMs"] | 120;
  config.timing.wifiReconnectIntervalMs =
      timing["wifiReconnectIntervalMs"] | 10000;
  config.timing.mqttReconnectIntervalMs =
      timing["mqttReconnectIntervalMs"] | 5000;
  config.timing.publishIntervalMs = timing["publishIntervalMs"] | 0;

  if (config.wifi.ssid.isEmpty() || config.mqtt.host.isEmpty() ||
      config.mqtt.clientId.isEmpty() || config.device.siteId.isEmpty() ||
      config.device.deviceId.isEmpty()) {
    printConfigError(F("campos obrigatorios de wifi/mqtt/device ausentes"));
    return false;
  }

  config.spotCount = spots.size();
  if (config.spotCount == 0) {
    printConfigError(F("spots vazio"));
    return false;
  }

  if (config.spotCount > MAX_SPOTS) {
    Serial.print(F("[Config] quantidade de spots excede MAX_SPOTS: "));
    Serial.println(MAX_SPOTS);
    return false;
  }

  for (size_t index = 0; index < config.spotCount; ++index) {
    if (!validateAndPopulateSpot(index, spots[index], config)) {
      return false;
    }
  }

  Serial.print(F("[Config] configuracao carregada com "));
  Serial.print(config.spotCount);
  Serial.println(F(" vagas"));
  return true;
}

bool ConfigManager::validateAndPopulateSpot(size_t index,
                                            const JsonObjectConst& spotObject,
                                            AppConfig& config) {
  if (spotObject.isNull()) {
    Serial.print(F("[Config] spot invalido no indice "));
    Serial.println(index);
    return false;
  }

  if (!spotObject["id"].is<const char*>() || !spotObject["pin"].is<uint8_t>() ||
      !spotObject["occupiedOnHigh"].is<bool>()) {
    Serial.print(F("[Config] campos obrigatorios ausentes no spot "));
    Serial.println(index);
    return false;
  }

  const String spotId = spotObject["id"].as<String>();
  if (spotId.isEmpty()) {
    Serial.print(F("[Config] id vazio no spot "));
    Serial.println(index);
    return false;
  }

  if (hasDuplicateId(config, index, spotId)) {
    Serial.print(F("[Config] id duplicado: "));
    Serial.println(spotId);
    return false;
  }

  config.spots[index].id = spotId;
  config.spots[index].pin = spotObject["pin"].as<uint8_t>();
  config.spots[index].occupiedOnHigh =
      spotObject["occupiedOnHigh"].as<bool>();
  return true;
}

bool ConfigManager::hasDuplicateId(const AppConfig& config, size_t spotIndex,
                                   const String& candidateId) const {
  for (size_t index = 0; index < spotIndex; ++index) {
    if (config.spots[index].id == candidateId) {
      return true;
    }
  }

  return false;
}

void ConfigManager::printConfigError(
    const __FlashStringHelper* message) const {
  Serial.print(F("[Config] "));
  Serial.println(message);
}
