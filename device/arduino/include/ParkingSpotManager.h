#pragma once

#include "ParkingSpot.h"

class ParkingSpotManager {
 public:
  void begin(const AppConfig& config, uint32_t nowMs);
  bool update(uint32_t nowMs);

  size_t getSpotCount() const;
  const ParkingSpot& getSpot(size_t index) const;

 private:
  ParkingSpot spots_[MAX_SPOTS];
  size_t spotCount_ = 0;
};
