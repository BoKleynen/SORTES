#include "database.h"
#include <EEPROM.h>

#define dbOffset 3 // sizeof(byte) + sizeof(nRecords)
#define maxRecords 256
#define queueLength 4
#define tempOffset 280.31
#define tempFactor 0.819672131 // 1 / 1.22
#define recordSize 2 // sizeof(unsigned int);
#define headAddress 1022
#define nRecordsAddress 1020

// Public functions
Database::Database(): head(-1), mutex(xSemaphoreCreateMutex()), queueHandle(xQueueCreate(queueLength, recordSize)), nRecords(0) {
  // create a writeTask with 100 words as stack and priority 2
  xTaskCreate(Database::writeTask, "WRITE TASK", 100, (void *) this, 2, NULL);
}

Database::Database(bool reset): mutex(xSemaphoreCreateMutex()), queueHandle(xQueueCreate(queueLength, recordSize)) {
  if (reset) {
    head = -1;
    nRecords = 0;
  } else {
    EEPROM.get(0, head);
    EEPROM.get(1, nRecords);
  }
  
  // create a writeTask with 100 words as stack and priority 2
  xTaskCreate(Database::writeTask, "WRITE TASK", 100, (void *) this, 2, NULL);
}

unsigned int Database::read(byte index) {
  unsigned int record;
  EEPROM.get(this->physicalAddress(index), record);
  return record;
}

/*
 * Doesn't need to acquire the mutex since reads and writes to EEPROM can never happen at the same time
 * and the task writing records can never be interrupted by a query to print the last record (this function) since it has higher priority.
 */
void Database::printLast(void) {
  Serial.print(F("record "));
  Serial.print(this->nRecords);
  Serial.print(F(": "));
  Serial.println(calcTemp(this->read(this->head)));
}

// Needs to acquire the mutex to ensure a new record won't be written in the middle of reading all records from EEPROM
void Database::printAll(void) {
  register int i;
  register int offset = maxRecords - 1 + this->head;
  unsigned int record;
  if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
  
    if (this->nRecords > maxRecords) {
      for (i = this->head + 1; i < maxRecords; i++) {
        Serial.print(F("record "));
        Serial.print(this->nRecords - offset + i);
        Serial.print(F(": "));
        Serial.println(calcTemp(this->read(i)));
      }
    }
  
    for (i = 0; i <= this->head; i++) {
      Serial.print(F("record "));
      Serial.print(this->nRecords - this->head + i);
      Serial.print(F(": "));
      Serial.println(calcTemp(this->read(i)));
    }

    xSemaphoreGive(this->mutex);
  }
}


void Database::write(unsigned int rec) {
  xQueueSendToBackFromISR(this->queueHandle, &rec, NULL);
}

// Private functions
static void Database::writeTask(void *dbArg) {
  auto db = (Database * const) dbArg;
  unsigned int record;

  for (;;) {
    if (xQueueReceive(db->queueHandle, &record, portMAX_DELAY) == pdTRUE && xSemaphoreTake(db->mutex, portMAX_DELAY) == pdTRUE) {
      EEPROM.put(db->physicalAddress(db->head + 1), record);
      db->incrementNRecords();
      db->incrementHead();
      xSemaphoreGive(db->mutex);
    }
    else
      vTaskDelay(4);
  }
}

/*
 * Converts the saved value to a temperature value in degrees Celsius
 */
static inline double Database::calcTemp(unsigned int value) {
  return (value - tempOffset ) * tempFactor;
}

/*
 * Calculates the physical address of the record at the given index
 */
inline int Database::physicalAddress(byte index) {
  return (int) index << 1;
}

/*
 * Increments the head pointer and writes the result to EEPROM.
 */
void Database::incrementHead(void) {
  this->head++;
  EEPROM.put(headAddress, this->head);
}

/*
 * Increments the amount of records that have been stored in te database
 */
void Database::incrementNRecords(void) {
  this->nRecords++;
  EEPROM.put(nRecordsAddress, this->nRecords);
}
