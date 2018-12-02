#ifndef database_h
#define database_h

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <Arduino.h>

/*
 * A class to store records in a database.
 * Records are 16 bit unsigned integers (only 10 will be used) with a position 
 * given by an 8 bit unsigned integer.
 * This limits the amount of records to 256.
 */
class Database {
  public:
    /*
     * Creates a default database with head pointing at -1, so the first record will be written to 0, 
     * nRecords initialised to 0.
     * Can be used to create a new database.
     */
    Database();

    /*
     * Creates a new Database object.
     * If useStored == true then the database will be recreated from EEPROM.
     * Otherwise a new Database is created with head pointing at -1 and
     * nRecords set to 0.
     */
    Database(bool useStored);

    /*
     * Writes a record to the database at the next position, 
     * increments `head` when done.
     */
    void write(unsigned int rec);

    /*
     * Prints all records currently stored in the database.
     */
    void printAll(void);

    /*
     * Prints the last record in the database.
     */
    void printLast(void);

  private:
    // stores the index of the last written record
    byte head;

    // amount of records that have been stored in the database; only the last 128 are available
    int nRecords;

    // semaphore handle
    SemaphoreHandle_t mutex;

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
