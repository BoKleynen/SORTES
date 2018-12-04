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

  // Reduce power usage
  PRR1 |= _BV(PRTIM3); // Disable timer 3
  PRR1 |= _BV(4); // Disable timer 4

  // create the realtimeTask with 100 words as stack and priority 3 (the highest used priority)
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
        sleepWhenAsked();
        break;
      case '3':
        db.printAll();
        break;
    }
  }
  sleepWhenIdle();
}

/*
 * Configures Timer 1 to interrupt every 500 ms
 */
void setupTimer() {
  cli();

  TCCR1A = 0; // set entire Timer 1 control register A to 0
  TCCR1B = 0; // set entire Timer 1 control register B to 0

  TCNT1  = 0; // initialize counter value to 0

  OCR1A = 7812; // set compare match register: 0.5s * (16 000 000 Hz / 1024)
  TCCR1B |= (1 << WGM12);   // turn on one of the Clear Timer on Compare (CTC) modes
  TCCR1B |= (1 << CS12) | (1 << CS10); // Enable 1024 prescaler

  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

  sei();
}

/*
 * ISR for the Timer 1 Output Compare A Match interrupt
 */
ISR(TIMER1_COMPA_vect) {
  db.write(getTemp());
}

/*
 * Enters IDLE sleep mode
 */
void sleepWhenIdle() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
  sleep_disable();
}

/*
 * Enters power down sleep mode
 */
void sleepWhenAsked() {
  DIDR0 = 0xF3;
  DIDR2 = 0x3F;

  ACSR &= ~_BV(ACIE);
  ACSR |= _BV(ACD);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  taskENTER_CRITICAL();
  vPortEndScheduler(); // wdt disable
  attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUpISR, RISING);
  sleep_enable();
  taskEXIT_CRITICAL();
  sleeping = true;
  sleep_mode(); // go to sleep
}

/*
 * ISR to wake up from sleep mode 
 */
void wakeUpISR() {
  wakeUp();
}

/*
 * Disables sleep mode
 */ 
void wakeUp() {
  sleeping = false;
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));

  wdt_reset();
  wdt_interrupt_enable( portUSE_WDTO );
  setupTimer();
}

/*
 * ISR to send airbag signal on collision
 */
void collisionISR(void) {
  xTaskResumeFromISR(realtimeTaskHandle); // Resume the suspended task.
  taskYIELD();
}

/*
 * The realtime task that deploys the airbag
 */
static void realtimeTask(void* pvParameters) {
  vTaskSuspend(realtimeTaskHandle); // Suspend the task
  
  if (sleeping)
    wakeUp(); // wake up
  else
    delay(4); // small delay in case the arduino was not sleeping
    
  vTaskDelay(10); // Wait for 10 ticks
  
  digitalWrite(airbagDeployement, HIGH);
  detachInterrupt(digitalPinToInterrupt(collisionDetector));
  
  vTaskDelete(realtimeTaskHandle); // Delete the task
}

/*
 * Returns the internal temperature reading
 */
unsigned int getTemp(void) {
  unsigned int wADC;

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0)); // http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
  ADCSRB |= _BV(MUX5);
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  while (bit_is_set(ADCSRA, ADSC)); // Detect end-of-conversion

  ADCSRA &= ~_BV(ADEN); // disable the ADC

  wADC = ADCL + (ADCH << 8);

  return wADC;
}
