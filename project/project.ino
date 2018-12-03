#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <EEPROM.h>
#include "database.h"
#include <avr/sleep.h>
#include <avr/wdt.h>

#define collisionDetector 3
#define airbagDeployement 12
#define wakeUpPin 2
#define databaseReset 5

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;


void collisionISR(void);
unsigned int getTemp();

Database db;
TaskHandle_t realtimeTaskHandle;

void setup() {
  //delay(2000);
  Serial.begin(115200);
  while (!Serial); // waits for serial to be available

  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }

  pinMode(databaseReset, INPUT);
  pinMode(airbagDeployement, OUTPUT);
  pinMode(wakeUpPin, INPUT_PULLUP);
  pinMode(collisionISR, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // setup database
  db = Database(digitalRead(databaseReset) == HIGH);
  setupTimer();

  //attachInterrupt(digitalPinToInterrupt(collisionDetector), collisionISR, LOW);


  xTaskCreate(realtimeTask, "Realtime Task", 100, NULL, 3, &realtimeTaskHandle);
}

// freeRTOS runs loop when no other task is available
void loop() {
  if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
  {
    if (Serial.available()) {
      switch (Serial.read()) {
        case '1':
          db.printLast();
          break;
        case '2':
          Serial.println("Serial: Entering Sleep mode");
          delay(100);           // this delay is needed, the sleep function will provoke a Serial error otherwise!!
          sleepWhenAsked();     // sleep function called here
          break;
        case '3':
          db.printAll();
          break;
      }
    }
    xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
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
  digitalWrite(LED_BUILTIN, LOW);
  if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
  {
    Serial.println("sleeping");

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    taskENTER_CRITICAL();
    vPortEndScheduler(); // wdt disable
    attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUpISR, LOW);
    sleep_enable();

    // sleep_bod_disable();
    taskEXIT_CRITICAL();
    sleep_mode();
    sleep_disable(); // disable sleep...
    detachInterrupt(digitalPinToInterrupt(wakeUpPin));
    Serial.println("Woke up");
    xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
  }
  wdt_reset();
  wdt_interrupt_enable( portUSE_WDTO );

  setupTimer();
  // vTaskStartScheduler();

  // wakeUpISR code will not be executed
  //  Serial.println("just woke up");
}

void wakeUpISR() {
  //Serial.println("in ISR");
  //sleep_disable();
  // detachInterrupt(digitalPinToInterrupt(wakeUpPin));
}

void collisionISR(void) {
  // Resume the suspended task.
  //Serial.println(F("Resuming Realtime task from ISR"));
  int xYieldRequired = xTaskResumeFromISR(realtimeTaskHandle);
  //Serial.println(xYieldRequired);
  if (xYieldRequired == 1)
  {
    taskYIELD();
  }
}

static void realtimeTask(void* pvParameters)
{
  vTaskSuspend(realtimeTaskHandle);
  //  Serial.println(F("Deploying airbag"));
  digitalWrite(airbagDeployement, HIGH);
  //  Serial.println(F("Back in realtime task and About to delete itself"));
  vTaskDelete(realtimeTaskHandle);    // Delete the task
}

// Temperature in degrees Celsius
unsigned int getTemp(void) {
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
  while (bit_is_set(ADCSRA, ADSC));

  ADCSRA &= ~_BV(ADEN);  // disable the ADC

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCL + (ADCH << 8);
  // Serial.println(wADC);

  return wADC;
}
