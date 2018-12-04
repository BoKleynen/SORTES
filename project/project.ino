#include <Arduino_FreeRTOS.h>
#include <EEPROM.h>
#include "database.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

#define collisionDetector 3
#define airbagDeployement 12
#define wakeUpPin 2
#define databaseReset 5

void collisionISR(void);
unsigned int getTemp();

bool sleeping = false;

Database db;
TaskHandle_t realtimeTaskHandle;

void setup() {
  while (!Serial); // waits for serial to be available
  Serial.begin(9600);

  pinMode(databaseReset, INPUT);
  pinMode(airbagDeployement, OUTPUT);
  pinMode(wakeUpPin, INPUT_PULLUP);
  pinMode(collisionISR, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(airbagDeployement, LOW);

  // setup database
  db = Database(digitalRead(databaseReset) == HIGH);
  setupTimer();

  attachInterrupt(digitalPinToInterrupt(collisionDetector), collisionISR, RISING);
  PRR1 |= _BV(PRTIM3); // Disable timer 3
  PRR1 |= _BV(4); // Disable timer 4

  xTaskCreate(realtimeTask, "Realtime Task", 100, NULL, 3, &realtimeTaskHandle);
}

// freeRTOS runs loop when no other task is available
void loop() {
  if (Serial.available()) {
    switch (Serial.read()) {
      case '1':
        db.printLast();
        break;
      case '2':
        // this delay is needed, the sleep function will provoke a Serial error otherwise!!
        // Without this the RX led stays lit
        delay(100);          
        sleepWhenAsked();     // sleep function called here
        break;
      case '3':
        db.printAll();
        break;
    }
  }
  sleepWhenIdle();
}

void setupTimer() {
  // Setup timer interrupts
  cli();//stop interrupts
  // p104 van docs
  //set timer1 interrupt at 2Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 2hz increments
  OCR1A = 7812;// = (16*10^6) / (2*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();//allow interrupts
}

ISR(TIMER1_COMPA_vect) {
  db.write(getTemp());
}

void sleepWhenIdle() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
  sleep_disable();
}

void sleepWhenAsked() {
  DIDR0 = 0xF3;
  DIDR2 = 0x3F;

  ACSR &= ~_BV(ACIE);
  ACSR |= _BV(ACD);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
  taskENTER_CRITICAL();
  vPortEndScheduler(); // wdt disable
  attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUpISR, RISING);
  sleep_enable();
  taskEXIT_CRITICAL();
  sleeping = true;
  sleep_mode();
}

void wakeUpISR() {
  wakeUp();
}

void wakeUp() {
  sleeping = false;
  sleep_disable(); // disable sleep...
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));
  
  wdt_reset();
  wdt_interrupt_enable( portUSE_WDTO );
  setupTimer();  
}

void collisionISR(void) {
  // Resume the suspended task.
  xTaskResumeFromISR(realtimeTaskHandle);
  taskYIELD();
}

static void realtimeTask(void* pvParameters) {
  vTaskSuspend(realtimeTaskHandle);
  if (sleeping) 
    wakeUp();
  else
    delay(4);
  vTaskDelay(10);
  digitalWrite(airbagDeployement, HIGH);  
  detachInterrupt(digitalPinToInterrupt(collisionDetector));
  vTaskDelete(realtimeTaskHandle);    // Delete the task
}

// Temperature in degrees Celsius
unsigned int getTemp(void) {
  unsigned int wADC;
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
  while (bit_is_set(ADCSRA, ADSC));

  ADCSRA &= ~_BV(ADEN);  // disable the ADC

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCL + (ADCH << 8);

  return wADC;
}
