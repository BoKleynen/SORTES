#include "database.h"
#include <EEPROM.h>

#define dbOffset 3 // sizeof(byte) + sizeof(nRecords)

// Public functions
Database::Database(byte head = -1, int nRecords = 0, unsigned int initialRecord = NULL): 
  head(head), 
  xSemaphore(xSemaphoreCreateBinary()),
  queueHandle(xQueueCreate(4, sizeof(unsigned int))),
  nRecords(nRecords)
{
    xTaskCreate(Database::writeTask, "WRITE TASK", 100, (void *) this, 2, NULL);
}

unsigned int Database::read(byte index) {
  unsigned int record;
  EEPROM.get(this->physicalAddress(index), record);
  return record;
}

void Database::printLast(void) {
  Serial.println(calcTemp(this->read(this->head)));
}

void Database::printAll(void) {
  int i;
  unsigned int record;
  if (this->nRecords > 256) {
    for (i = this->head + 1; i < 256; i++) {
      Serial.println(calcTemp(this->read(i)));
    }
  }

  for (i = 0; i <= this->head; i++) {
    Serial.println(calcTemp(this->read(i)));
  }
}


void Database::write(unsigned int rec) {
  Serial.println(F("write function"));
  xQueueSendToBackFromISR(this->queueHandle, &rec, NULL);
}

// Private functions
static void Database::writeTask(void *dbArg) {
  auto db = (Database *const) dbArg;
  unsigned int record;
  Serial.println(db->head);
  
  for(;;) {
    if(xQueueReceive(db->queueHandle, &record, portMAX_DELAY)) {
      delay(10);
      Serial.println("wirte task: ");
      Serial.println(*record); 
//      EEPROM.put(a->db->physicalAddress(a->db->head + 1), a->rec);
//      a->db->incrementNRecords();
//      a->db->incrementHead();
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
