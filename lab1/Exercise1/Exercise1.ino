
void blink(int);

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  String inputOn;

  delay(2000);
  Serial.println("Enter LED status (on/off):");
  
  
  while (Serial.available() == 0) {
    delay(100);
  }
  
  inputOn = Serial.readStringUntil('\n');

  if (inputOn == "on") {
    int blinkRate;
    
    Serial.println("Enter the blink rate (1-60 sec):");
    Serial.flush();

    while (Serial.available() == 0) {
      delay(100);
    }
  
    blinkRate = Serial.readStringUntil('\n').toInt();
    Serial.println("You have selected LED on. Blink rate is " + String(blinkRate) + " sec." );
    blink(blinkRate * 1000);
  } else {
        Serial.println("You have selected LED off." );
  }
}

void blink(int interval) {
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(interval);
    digitalWrite(LED_BUILTIN, LOW);
    delay(interval);
  }
}
