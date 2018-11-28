#include <Arduino_FreeRTOS.h>
#include <EEPROM.h>
#include "database.h"

const byte reset = 5;
const byte collisionDetector = 7;
const byte airbagDeployement = 13;

void collisionISR(void);
double getTemp();

Database db;
TaskHandle_t realtimeTaskHandle;

void setup() {
  pinMode(reset, INPUT);
  pinMode(airbagDeployement, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(collisionDetector), collisionISR, RISING);

  // setup database
  if (digitalRead(reset) == HIGH) {
    db = Database();
  } else {
    int nRecords;
    byte head;
    EEPROM.get(1, nRecords);
    EEPROM.get(0, head);
    db = Database(head, nRecords);
  }

  // TODO: store 1 record in setup

  Serial.begin(9600);
  while(!Serial); // waits for serial to be available

  xTaskCreate(realtimeTask, "Realtime Task", 100, NULL, 3, &realtimeTaskHandle);
}

// freeRTOS runs loop when no other task is available
void loop() {
  delay(1000);
  Serial.println(getTemp(), 1);
}

void collisionISR(void) {
  // Resume the suspended task.
  //Serial.println(F("Resuming Realtime task from ISR"));
  int xYieldRequired = xTaskResumeFromISR(realtimeTaskHandle);
  //Serial.println(xYieldRequired);
  if(xYieldRequired == 1)
  {
     taskYIELD();
  }
}

static void realtimeTask(void* pvParameters)
{
  Serial.println(F("Realtime task Running"));
  vTaskSuspend(realtimeTaskHandle);
  Serial.println(F("Deploying airbag"));
  digitalWrite(airbagDeployement, HIGH);
  Serial.println(F("Back in realtime task and About to delete itself"));
  vTaskDelete(realtimeTaskHandle);    // Delete the task
}

// Temperature in degrees Celsius
double getTemp(void) {
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0)); // http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
  ADCSRB |= _BV(MUX5);
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  ADCSRA ^= _BV(ADEN);  // disable the ADC

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCL + (ADCH << 8);
  // Serial.println(wADC);
  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 280.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
