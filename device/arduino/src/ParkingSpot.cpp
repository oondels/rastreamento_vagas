#include "ParkingSpot.h"

#include <Arduino.h>

void ParkingSpot::begin(const ParkingSpotDefinition& definition,
                        uint32_t debounceMs, uint32_t nowMs) {
  definition_ = definition;
  debounceMs_ = debounceMs;

  pinMode(definition_.pin, INPUT);

  rawOccupied_ = readOccupiedRaw();
  stableOccupied_ = rawOccupied_;
  lastRawChangeMs_ = nowMs;
  initialized_ = true;
}

bool ParkingSpot::update(uint32_t nowMs) {
  if (!initialized_) {
    return false;
  }

  const bool currentRaw = readOccupiedRaw();
  if (currentRaw != rawOccupied_) {
    rawOccupied_ = currentRaw;
    lastRawChangeMs_ = nowMs;
  }

  if (stableOccupied_ != rawOccupied_ &&
      nowMs - lastRawChangeMs_ >= debounceMs_) {
    stableOccupied_ = rawOccupied_;
    return true;
  }

  return false;
}

const String& ParkingSpot::getId() const { return definition_.id; }

uint8_t ParkingSpot::getPin() const { return definition_.pin; }

bool ParkingSpot::isOccupied() const { return stableOccupied_; }

bool ParkingSpot::readOccupiedRaw() const {
  const bool pinHigh = digitalRead(definition_.pin) == HIGH;
  return definition_.occupiedOnHigh ? pinHigh : !pinHigh;
}
