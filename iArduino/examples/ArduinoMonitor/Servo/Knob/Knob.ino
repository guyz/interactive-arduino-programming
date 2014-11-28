// This example shows how to use a servo with iArduinoTerminal

// Controlling a servo position using a potentiometer (variable resistor) 
// by Michal Rinott <http://people.interaction-ivrea.it/m.rinott> 

#include <EEPROM.h>             // include EEPROM.h even if you don't need it
#include <Servo.h>              // include Servo.h even if you don't need it
#include <iArduino.h>           // include library header

iArduinoHandleProtocol debug;   // declare iArduinoHandleProtocol
 
// You don't need to declare here
//Servo myservo;  // create servo object to control a servo 
 
int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin 
 
void setup() 
{ 
  debug.begin();               // Setup, serial port is initialized. 
                               // The baudrate is 115.2kbps.
  // Use servo[0]...servo[11]
  servo[0].attach(9);  // attaches the servo on pin 9 to the servo object 
} 
 
void loop() 
{ 
  debug.check();               // Periodiclly call this function

  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 179);     // scale it to use it with the servo (value between 0 and 180) 
  servo[0].write(val);                  // sets the servo position according to the scaled value 
  delay(15);                           // waits for the servo to get there 
} 
