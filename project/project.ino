#include <Arduino_FreeRTOS.h>

const byte collisionDetector = 7;
const byte airbagDeployement = 13;

void collisionISR(void);
double getTemp();

void setup() {
  pinMode(airbagDeployement, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(collisionDetector), collisionISR, RISING);

  Serial.begin(9600);
  while(!Serial); // waits for serial to be available
}

// freeRTOS runs loop when no other task is available
void loop() {
  delay(1000);
  Serial.println(getTemp(), 1);
}

void collisionISR(void) {
  digitalWrite(airbagDeployement, HIGH);
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
  //Serial.println(wADC);
  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 280.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
