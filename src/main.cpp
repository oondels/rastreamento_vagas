#include <Arduino.h>

constexpr uint8_t sensorPins[] = {33, 25, 26, 27};
constexpr size_t sensorCount = sizeof(sensorPins) / sizeof(sensorPins[0]);

bool lastSensorStates[sensorCount] = {false, false, false, false};

void setup() {
  Serial.begin(115200);

  for (size_t i = 0; i < sensorCount; i++) {
    pinMode(sensorPins[i], INPUT);
  }
}

void loop() {
  for (size_t i = 0; i < sensorCount; i++) {
    bool currentState = digitalRead(sensorPins[i]) == HIGH;

    if (currentState && !lastSensorStates[i]) {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.println(" detectado");
    }

    lastSensorStates[i] = currentState;
  }

  delay(50);
}
