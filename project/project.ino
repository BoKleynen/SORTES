#include <Arduino_FreeRTOS.h>

const byte collisionDetector = 7;
const byte airbagDeployement = 13;

void collisionISR(void);

void setup() {
  pinMode(airbagDeployement, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(collisionDetector), collisionISR, RISING);

  Serial.begin(9600);
}

// freeRTOS runs loop when no other task is available
void loop() {

}

void collisionISR(void) {
  digitalWrite(airbagDeployement, HIGH);
}
