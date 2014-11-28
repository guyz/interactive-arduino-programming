#include <EEPROM.h>             // include EEPROM.h even if you don't need it
#include <Servo.h>              // include Servo.h even if you don't need it
#include <iArduino.h>           // include library header

iArduinoHandleProtocol debug;   // declare iArduinoHandleProtocol

void setup() {
  debug.begin();               // initialize iArduinoHandleProtocol
  			       // Serial port is initialized at 
                               // 115.2kbps.
  // put your setup code here, to run once:

}

void loop() {
  debug.check();               // call this function periodically
  // put your main code here, to run repeatedly: 
  
}
