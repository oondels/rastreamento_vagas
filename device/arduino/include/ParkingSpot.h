#pragma once

#include "AppConfig.h"

class ParkingSpot {
 public:
  ParkingSpot() = default;

  void begin(const ParkingSpotDefinition& definition, uint32_t debounceMs,
             uint32_t nowMs);
  bool update(uint32_t nowMs);

  const String& getId() const;
  uint8_t getPin() const;
  bool isOccupied() const;

 private:
  bool readOccupiedRaw() const;

  ParkingSpotDefinition definition_;
  uint32_t debounceMs_ = 0;
  bool initialized_ = false;
  bool rawOccupied_ = false;
  bool stableOccupied_ = false;
  uint32_t lastRawChangeMs_ = 0;
};
