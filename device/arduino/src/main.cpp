#include <Arduino.h>

#include "ParkingController.h"

ParkingController parkingController;

void setup() { parkingController.begin(); }

void loop() { parkingController.update(); }
