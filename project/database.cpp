#include "database.h"
#include <EEPROM.h>
#include <Arduino_FreeRTOS.h>

const dbOffset = 1

// Public functions

Database::Database(byte index = 1): index(index), xSemaphore(xSemaphoreCreateBinary()) {
  if (xSemaphore == NULL) {
    // todo: signal something went horribly wrong
  }
}

/*
 * Reads the last record of the database in the EEPROM memory.
 * To read the last value we dont need to acquire a lock since the head is only incremented after a write.
 */
float Database::readLast() {
  return this->read(this->head);
}

/*
 * args:
 *    res: pointer to an array that can hold at least 128 records, 
 *         this function will return the actual amount of records written
 */
byte Database::readAll(float *res) {
  int i;
  byte nRecords = 0;
  if (this->nRecords > 128) {
    for (i=this->head+1; i <= 127; i++) {
      EEPROM.get(this->physicalAddress(i), f[i])
    }

    nRecords += 128 - (this->head + 1);
  }

  for (i=0; i <= this->head; i++) {
    EEPROM.get(this->physicalAddress(i), f[i])
  }

  nRecords += (this->head + 1);
}

void Database::write() {
  xTaskCreate(this->_write, "DB WRITE", 10, (void) args, 2, NULL)
}

// Private functions
inline int Database::physicalAddress(byte index) {
  return ((int) index << 2) + dbOffset;
}

void Database::incrementHead() {
  this->head = (this->head + 1) & 127;
  EEPROM.put(0, this->head);
}

void Database::_write() {
  if (xSemaphoreTake(this->xSemaphore, portMAX_DELAY) == pdTRUE) {
    EEPROM.put(this->physicalAddress(this->head + 1), value);
    this->incrementHead();
    xSemaphoreGive(this->xSemaphore);
    vTaskDelete(NULL);
  }
}
