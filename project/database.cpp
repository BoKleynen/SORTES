#include "database.h"
#include <EEPROM.h>
#include <Arduino_FreeRTOS.h>

#define dbOffset 1

// Public functions

Database::Database(byte head = 0, int nRecords = 0): head(head), xSemaphore(xSemaphoreCreateBinary()), nRecords(nRecords) {
  if (xSemaphore == NULL) {
    ;
  }
}

/*
 * Reads the last record of the database in the EEPROM memory.
 * To read the last value we dont need to acquire a lock since the head is only incremented after a write.
 */
float Database::readLast() {
  float record;
  EEPROM.get(this->physicalAddress(this->head), record);
  return record;
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
      EEPROM.get(this->physicalAddress(i), res[i]);
    }

    nRecords += 128 - (this->head + 1);
  }

  for (i=0; i <= this->head; i++) {
    EEPROM.get(this->physicalAddress(i), res[i]);
  }
}

struct Args {
  Database *db;
  float rec;
};

void Database::write(float rec) {
  Args *args = new Args { this, rec };
  xTaskCreate([] (void *_args) -> void {
    Args *args = (Args *) _args;
    if (xSemaphoreTake(args->db->xSemaphore, portMAX_DELAY) == pdTRUE) {
      EEPROM.put(args->db->physicalAddress(args->db->head + 1), args->rec);
      args->db->nRecords++;
      args->db->incrementHead();
      xSemaphoreGive(args->db->xSemaphore);
      vTaskDelete(NULL);
    }
  }, 
  "DB WRITE", 10, (void *) args, 2, NULL);
}

// Private functions
inline int Database::physicalAddress(byte index) {
  return ((int) index << 2) + dbOffset;
}

void Database::incrementHead() {
  this->head = (this->head + 1) & 127;
  EEPROM.put(0, this->head);
}
