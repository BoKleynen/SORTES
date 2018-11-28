#include "database.h"
#include <EEPROM.h>
#include <Arduino_FreeRTOS.h>

#define dbOffset 3 // sizeof(byte) + sizeof(nRecords)


inline double calcTemp(unsigned int value){
  return (value - 280.31 ) / 1.22;
}

inline void print(unsigned int value){
  Serial.println(calcTemp(value));
}

// Public functions
Database::Database(byte head = -1, int nRecords = 0): head(head), xSemaphore(xSemaphoreCreateBinary()), nRecords(nRecords) {
  if (xSemaphore == NULL) {
    ;
  }
}

unsigned int Database::read(byte index) {
  unsigned int record;
  EEPROM.get(this->physicalAddress(index), record);
  return record;
}

void Database::printLast(void) {
  Serial.println("Reading last");
  print(this->read(this->head));
}

void Database::printAll(void) {
  int i;
  unsigned int record;
  if (this->nRecords > 256) {
    for (i=this->head+1; i < 256; i++) {
      print(this->read(i));
    }
  }

  for (i=0; i <= this->head; i++) {
    print(this->read(i));
  }
}

struct Args {
  Database *db;
  unsigned int rec;
};

void Database::write(unsigned int rec) {  
  Args *args = &Args {this, rec};
  xTaskCreate(this->writeTask, 
  "DB WRITE", 
  10, 
  (void *) args, 
  2, 
  NULL);
}

// Private functions
inline int Database::physicalAddress(byte index) {
  return ((int) index << 1) + dbOffset;
}

void Database::incrementHead() {
  this->head++;
  EEPROM.put(0, this->head);
}

void Database::incrementNRecords() {
  this->nRecords++;
  EEPROM.put(1, this->nRecords);
}

static void Database::writeTask(void *args) {
  Args *a = (Args *) args;
  if (xSemaphoreTake(a->db->xSemaphore, portMAX_DELAY) == pdTRUE) {
      EEPROM.put(a->db->physicalAddress(a->db->head + 1), * ((unsigned int*) args));
      a->db->incrementNRecords();
      a->db->incrementHead();
      xSemaphoreGive(a->db->xSemaphore);
      vTaskDelete(NULL);
    }
}
