// This example shows how to use a servo with iArduinoTerminal
//
// Sweep
// by BARRAGAN <http://barraganstudio.com> 
// This example code is in the public domain.

#include <EEPROM.h>             // include EEPROM.h even if you don't need it
#include <Servo.h>              // include Servo.h even if you don't need it
#include <iArduino.h>           // include library header

iArduinoHandleProtocol debug;   // declare iArduinoHandleProtocol
 
// You don't need to declare here
//Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
 
int pos = 0;    // variable to store the servo position 
 
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

  for(pos = 0; pos < 180; pos += 1)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    servo[0].write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  for(pos = 180; pos>=1; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    servo[0].write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
} 
