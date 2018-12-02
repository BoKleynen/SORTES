#include "database.h"
#include <EEPROM.h>

#define dbOffset 3 // sizeof(byte) + sizeof(nRecords)

// Public functions
Database::Database(): 
  head(-1),
  xSemaphore(xSemaphoreCreateBinary()),
  queueHandle(xQueueCreate(4, sizeof(unsigned int))),
  nRecords(0)
{
  xTaskCreate(Database::writeTask, "WRITE TASK", 100, (void *) this, 2, NULL);
}

Database::Database(bool useStored):
  xSemaphore(xSemaphoreCreateBinary()),
  queueHandle(xQueueCreate(4, sizeof(unsigned int)))
{
  if (useStored) {
    EEPROM.get(0, head);
    EEPROM.get(1, nRecords);
  } else {
    head = -1;
    nRecords = 0;
  }
  xTaskCreate(Database::writeTask, "WRITE TASK", 100, (void *) this, 2, NULL);
}

unsigned int Database::read(byte index) {
  unsigned int record;
  EEPROM.get(this->physicalAddress(index), record);
  return record;
}

void Database::printLast(void) {
  Serial.print(F("record "));
  Serial.print(this->nRecords);
  Serial.print(F(": "));
  Serial.println(calcTemp(this->read(this->head)));
}

void Database::printAll(void) {
  register int i;
  register int a = 255 + this->head;
  unsigned int record;
  Serial.println(this->head);
  Serial.println(this->nRecords);
  if (this->nRecords > 256) {
    for (i = this->head + 1; i < 256; i++) {
      Serial.print(F("record "));
      Serial.print(this->nRecords - a + i);
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
}


void Database::write(unsigned int rec) {
  xQueueSendToBackFromISR(this->queueHandle, &rec, NULL);
}

// Private functions
static void Database::writeTask(void *dbArg) {
  auto db = (Database * const) dbArg;
  unsigned int record;

  for (;;) {
    if (xQueueReceive(db->queueHandle, &record, portMAX_DELAY) == pdTRUE) {
      delay(10);  // not sure why but doesn't work without
      EEPROM.put(db->physicalAddress(db->head + 1), record);
      db->incrementNRecords();
      db->incrementHead();
      delay(10); // not sure why but doesn't work without
    }
  }
}

static inline double Database::calcTemp(unsigned int value) {
  return (value - 280.31 ) / 1.22;
}

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
