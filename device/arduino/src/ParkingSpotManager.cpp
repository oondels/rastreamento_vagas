#include "ParkingSpotManager.h"

void ParkingSpotManager::begin(const AppConfig& config, uint32_t nowMs) {
  spotCount_ = config.spotCount;
  for (size_t index = 0; index < spotCount_; ++index) {
    spots_[index].begin(config.spots[index], config.timing.sensorDebounceMs,
                        nowMs);
  }
}

bool ParkingSpotManager::update(uint32_t nowMs) {
  bool hasStableChange = false;
  for (size_t index = 0; index < spotCount_; ++index) {
    if (spots_[index].update(nowMs)) {
      hasStableChange = true;
    }
  }

  return hasStableChange;
}

size_t ParkingSpotManager::getSpotCount() const { return spotCount_; }

const ParkingSpot& ParkingSpotManager::getSpot(size_t index) const {
  return spots_[index];
}
