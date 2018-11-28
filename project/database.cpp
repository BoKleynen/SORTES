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

float Database::read(byte index) {
  float record;
  EEPROM.get(this->physicalAddress(index), record);
  return record;
}

void Database::printLast(void) {
  Serial.println(this->read(this->head));
}

void Database::printAll(void) {
  int i;
  float record;
  if (this->nRecords > 128) {
    for (i=this->head+1; i <= 127; i++) {
      Serial.println(this->read(i));
    }

    nRecords += 128 - (this->head + 1);
  }

  for (i=0; i <= this->head; i++) {
    Serial.println(this->read(i));
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
      delete args;
      vTaskDelete(NULL);
    }
  }, 
  "DB WRITE", 
  10, 
  (void *) args, 
  2, 
  NULL);
}

// Private functions
inline int Database::physicalAddress(byte index) {
  return ((int) index << 2) + dbOffset;
}

void Database::incrementHead() {
  this->head = (this->head + 1) & 127;
  EEPROM.put(0, this->head);
}
