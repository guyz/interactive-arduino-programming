/* 
   This sketch shows an example use of iArduinoTerminal for debugging without
   using iArduino language. The sketch extends Blink without Delay in the example. 
 */

#include <EEPROM.h>             // include EEPROM.h even if you don't need it
#include <Servo.h>              // include Servo.h even if you don't need it
#include <iArduino.h>           // include library header

iArduinoHandleProtocol debug;   // declare iArduinoHandleProtocol

const int ledPin =  13;
int ledState = LOW;
long previousMillis = 0;
long interval = 1000;

void setup() {
  debug.begin();               // Setup, serial port is initialized. 
                               // The baudrate is 115.2kbps.
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  debug.check();               // Periodiclly call this function

  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;   

    if (ledState == LOW) {
      ledState = HIGH;
      Serial.println("HIGH");  // You can use Serial.flush, print, println, and write
                               // as usual.
    } else {
      ledState = LOW;
      Serial.println("LOW");
    }

    digitalWrite(ledPin, ledState);
  }

  if (debug.available()) {    // Use available() of iArduinoHandleProtcol instead of
                              // Serial.available()
    int c = debug.read();     // Use read() of iArduinoHandleProtcol instead of 
                              // Serial.read();
    if (c == '1')
      Serial.println("Recieved 1.");
  }
}
