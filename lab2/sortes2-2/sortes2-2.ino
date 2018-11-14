#include <EEPROM.h>

int latestKey;

struct Record {
  int primaryKey;
  double temperature;
};

void setup() {
  Serial.begin(9600);
  delay(2000); 
}

void loop() {
  Serial.println
  if (Serial.available() == 0) {
    delay(5000);
    struct Record record = {latestKey, 20.0};
    EEPROM.put(++latestKey * sizeof(struct Record), record);
    EEPROM.put(0, latestKey);
  } else {
    String input = Serial.readStringUntil('\n');
    if (input == "last") {
      struct Record record;
      EEPROM.get((latestKey-1) * sizeof(struct Record), record);
      Serial.println("Record: ");
      Serial.println(record.primaryKey);
      Serial.println(record.temperature);
    }
  }
}
