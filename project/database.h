#include <Arduino_FreeRTOS.h>

class Database {
public:
  Database(byte);
  float readLast();
  byte readAll(float *res)
  void write(const *byte rec);
  
private:
  // stores the index of the last written record
  byte head;
  
  // amount of records that have been stored in the database; only the last 128 are available
  int nRecords;
  
  // semaphore handle
  SemaphoreHandle_t xSemaphore;
  
  inline int physicalAddress(byte index);
  inline int headAddress();
  void incrementHead();
  void _write();
};
