int input = 7;
 
bool conn = false;
  
void setup() {


  pinMode(input, INPUT);
  
  Serial.begin(9600);
}

void loop() {
  delay(1000);
  
  if (!conn && digitalRead(input) == HIGH) {
    Serial.println("pin connection detected");
    conn = true;
  } else if (conn && digitalRead(input) == LOW) {
    Serial.println("pin connection removed");
    conn = false;
  } 
}
