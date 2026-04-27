#pragma once

#include <ArduinoJson.h>

#include "AppConfig.h"

class ConfigManager {
 public:
  bool beginAndLoad(AppConfig& config);

 private:
  bool loadConfigFromFile(AppConfig& config);
  bool validateAndPopulateSpot(size_t index, const JsonObjectConst& spotObject,
                               AppConfig& config);
  bool hasDuplicateId(const AppConfig& config, size_t spotIndex,
                      const String& candidateId) const;
  void printConfigError(const __FlashStringHelper* message) const;
};
