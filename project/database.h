#ifndef database_h
#define database_h

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <Arduino.h>

class Database {
  public:
    Database();
    Database(byte head, int nRecords);
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

    // Queue handle
    QueueHandle_t queueHandle;

    static void writeTask(void *args);
    static inline double calcTemp(unsigned int value);
    inline int physicalAddress(byte index);
    inline int headAddress();
    void incrementHead();
    void incrementNRecords();
    unsigned int readLast();
    unsigned int read(byte index);
};

#endif
