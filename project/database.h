#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

class Database {
public:
  Database(byte head = -1, int nRecords = 0);
  unsigned int readLast();
  unsigned int read(byte index);
  void write(unsigned int rec);
  void printAll(void);
  void printLast(void);
private:
  // stores the index of the last written record
  byte head;
  
  // amount of records that have been stored in the database; only the last 128 are available
  int nRecords;
  
  // semaphore handle
  SemaphoreHandle_t xSemaphore;
  
  inline int physicalAddress(byte index);
  inline int headAddress();
  static void writeTask(void *args);
  void incrementHead();
  void incrementNRecords();
};
