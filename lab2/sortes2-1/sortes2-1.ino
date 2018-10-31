void setup()
{
  Serial.begin(9600);

  Serial.println(F("Internal Temperature Sensor"));
}

void loop()
{
  // Show the temperature in degrees Celcius.
  Serial.println(GetTemp(),1);
}

double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0)); // http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
  ADCSRB |= _BV(MUX5);
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCL + (ADCH << 8);
  //Serial.println(wADC);
  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 280.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
