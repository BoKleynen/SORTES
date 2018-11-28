#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

class Database {
public:
  Database(byte head = 0, int nRecords = 0);
  float readLast();
  byte readAll(float *res);
  void write(float rec);
  
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
