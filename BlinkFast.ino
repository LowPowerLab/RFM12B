// test save as

/*
  BlinkFast
  Turns on an LED on for 100 ms, then off for 100 ms, repeatedly.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait 100 ms
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait 100 ms
}
