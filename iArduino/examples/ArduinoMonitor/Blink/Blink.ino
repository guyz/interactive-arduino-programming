/*
   This sketch shows an example use of iArduinoTerminal for debugging without
   using iArduino language. The sketch extends Blink in the example. 

  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

#include <EEPROM.h>             // include EEPROM.h even if you don't need it
#include <Servo.h>              // include Servo.h even if you don't need it
#include <iArduino.h>           // include library header

iArduinoHandleProtocol debug;   // declare iArduinoHandleProtocol

void setup() {                
  debug.begin();               // initialize iArduinoHandleProtocol
  			       // Serial port is initialized at 
                               // 115.2kbps.
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(13, OUTPUT);     
}

void loop() {
  debug.check();            // call this function periodically

  digitalWrite(13, HIGH);   // set the LED on
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // set the LED off
  delay(1000);              // wait for a second
}
