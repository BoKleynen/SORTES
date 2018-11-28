#include <avr/sleep.h>

double getTemp();

//storage variables
boolean toggle1 = 0;

void setup(){
  while(!Serial);
  
  //set pins as outputs
  pinMode(13, OUTPUT);

cli();//stop interrupts

// p104 van docs
//set timer1 interrupt at 2Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 2hz increments
  OCR1A = 7812;// = (16*10^6) / (2*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

sei();//allow interrupts
}//end setup

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1Hz toggles pin 13 (LED)
  Serial.println("Timer 1: Write temperature");
  if (toggle1){
    digitalWrite(13,HIGH);
    toggle1 = 0;
  }
  else{
    digitalWrite(13,LOW);
    toggle1 = 1;
  }
SMCR &= ~_BV(SE);
}  


void loop(){
  //do other things here
  Serial.println("sleeping");
  cli();
  SMCR &= ~_BV(SM0);
  SMCR |= _BV(SM1);
  SMCR &= ~_BV(SM2);
  SMCR |= _BV(SE);
  sei();
  sleep_cpu();
  Serial.println("just woke up");
}
