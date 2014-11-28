// A C interpreter for Arduino
// Copyright(c) 2012 Noriaki Mitsunaga.  All rights reserved.
//
// This is a free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// See file LICENSE.txt for further informations on licensing terms.
//

#ifndef __IARDUINO_H__
#define __IARDUINO_H__
#include "Arduino.h"
// #define __NO_SERVO__
#if !defined(__NO_SERVO__)
#include <Servo.h>
#endif

void iArduinoSetup(char *, unsigned short len);
void iArduinoLoop();

#if !defined(__NO_SERVO__)
class myServo {
 public:
  void attach(int pin);
  void detach();
  void write(int angle);
 private:
  Servo s;
  unsigned char pin, idx;
};

extern myServo servo[12];
#endif

// Wrapper functions
int analogRead_(uint8_t pin);
int analogWrite_(int pin, int val);
int delay_(int ms);
int digitalRead_(int pin);
int digitalWrite_(int pin, int val);
int pinMode_(int pin, int mode);

// Replace normal functions with wrapper functions
#define analogRead(x)      analogRead_(x)
#define analogWrite(x, y)  analogWrite_(x, y)
#define delay(x)           delay_(x)
#define digitalRead(x)     digitalRead_(x)
#define digitalWrite(x, y) digitalWrite_(x, y)
#define pinMode(x, y)      pinMode_(x, y)

class iArduinoHandleProtocol {
 public:
  bool available();
  void begin() {begin(true);};
  void begin(bool verbose);
  void check();
  int read();
};

#endif
