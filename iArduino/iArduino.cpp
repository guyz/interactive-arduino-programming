// A C interpreter for Arduino
// Copyright(c) 2012 Noriaki Mitsunaga.  All rights reserved.
//
// This is a free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// See file LICENSE.txt for further informations on licensing terms.
//
// $Id: iArduino.ino,v 1.7 2012/03/08 10:52:23 kurobox Exp kurobox $
// See http://n.mtng.org/ele/arduino/iarduino.html (in English)
// See http://n.mtng.org/ele/arduino/iarduino-j.html (in Japanese)

#define ARDUINO
#ifdef ARDUINO
#include "Arduino.h"
#include <Hardwareser->h>
#include <SoftwareSerial.h>
#include "EEPROM.h"
#include "iArduinoTerminal.h"
#else
#include <stdio.h>
#include <windows.h>
#endif

#include "iArduino.h"
#if !defined(__NO_SERVO__)
#include "Servo.h"
#endif

#if defined(__AVR_ATmega32U4__)
#define ARDUINO_LEONARD
#define ANALOG_PINS 12
#else
#define ANALOG_PINS 6
#endif

//////////////////////////////////////////////////////////////////////////////
// Definitions of enums & structs
//////////////////////////////////////////////////////////////////////////////
#define INPUT_LEN 80   // Size of input buffer
#define SERIAL_PORT_FLAG 0x117
#define SERIAL_PORT_RX 0x118
#define SERIAL_PORT_TX 0x119

enum ERRORS {
  STOPPED = 10,
  FOUND_CONTINUE = 1,
  FOUND_BREAK = 2,
  ERROR_DIV0 = -1, 
  ERROR_SYNTAX = -2,
  ERROR_INTERNAL = -3
};

struct ArduinoConstValues {
  char *name;
  int   len;
  int   val;
};

struct Function1 {
  char *name;
  int   len;
  int (*func)(int);
};

struct Function2 {
  char *name;
  int   len;
  int (*func)(int, int);
};

struct Function3 {
  char *name;
  int   len;
  int (*func)(int, int, int);
};

///////////////////////////////////////////////////////////////////////////
// Fucntion declarations
///////////////////////////////////////////////////////////////////////////
#ifndef ARDUINO
#define getcharS() getchar()
#else
static byte getcharS();
int analogRead__(int pin) {
  return analogRead_(pin);
} 
#endif
static void HandleProtocol();
static int  eval2(char *s, int len, int *val);
static int print(int a);
static void printError(int err);
static void printSln(const char *s);
static void ReadProg();
static void SaveProg();
static void sendProgPos(unsigned short s, unsigned short len);
static int stepRun();
static void edit(char *prog, int lNum, char *line);
static int  eval2(char *s, int len, int *val);
static void getsS(char *buf, size_t len);
static void printError(int err);
static int rand(int);
static int  run(char *prog, int len);
static char *skipSpace(char *s, char *e);
static void list(char *prog);
static void printS(const char *s);

// Undef wrappers
#undef analogRead
#undef analogWrite
#undef delay
#undef digitalRead
#undef digitalWrite
#undef pinMode

// Wrapper functions
int max_(int a, int b);
int millis_(int dummy);
int min_(int a, int b);
int noTone_(int pin);
int tone2(int pin, int freq);
int tone3(int pin, int freq, int duration);

//////////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////////
int variables[26] = {0};

const struct ArduinoConstValues constants[] = {
  {"LOW", 3, 0},
  {"HIGH", 4, 1},
  {"false", 5, 0},
  {"true", 4, 1},
  {"INPUT", 5, 0},
  {"OUTPUT", 6, 1},
  {"A0", 2, 14},
  {"A1", 2, 15},
  {"A2", 2, 16},
  {"A3", 2, 17},
  {"A4", 2, 18},
  {"A5", 2, 19},
#ifdef ARDUINO_LEONARD
  {"A6", 2, 20},
  {"A7", 2, 21},
  {"A8", 2, 22},
  {"A9", 2, 23},
  {"A10", 2, 24},
  {"A11", 2, 25},
#endif
};

// ----- Function tables -----
#ifndef ARDUINO
const struct Function1 func1[] = {
  {"rand(", 5, rand_},
  {"print(", 6, print},
};

const struct Function2 func2[] = {
  {"plus(", 5, plus},
};
#else /* ARDUINO */
const struct Function1 func1[] = {
  {"abs(", 4, abs},
  {"analogRead(", 11, analogRead__},
  {"delay(", 6, delay_},
  {"digitalRead(", 12, digitalRead_},
  {"millis(", 7, millis_},
  {"noTone(", 7, noTone_},
  {"rand(", 5, rand},
  {"print(", 6, print},
//  {"sq(", 3, sq},
//  {"sqrt(", 5, sqrt},
};

const struct Function2 func2[] = {
  {"analogWrite(", 12, analogWrite_},
  {"digitalWrite(", 13, digitalWrite_},
  {"min(", 4, min_},
  {"max(", 4, max_},
  {"pinMode(", 8, pinMode_},
  {"tone(", 5, tone2},
//  {"plus(", 5, plus},
};

const struct Function3 func3[] = {
  {"tone3(", 5, tone3}
};
#endif

#define CONST_NUM (sizeof(constants)/sizeof(constants[0]))
#define FUNC1_NUM (sizeof(func1)/sizeof(func1[0]))
#define FUNC2_NUM (sizeof(func2)/sizeof(func2[0]))
#define FUNC3_NUM (sizeof(func3)/sizeof(func3[0]))

#ifndef ARDUINO_LEONARD
static unsigned char analogPinConf = 0;
#else
static unsigned short analogPinConf = 0x0fc0;
#endif
static unsigned short pwmPinConf = 0;
static unsigned char pwmValues[16], servoValues[12], servoPins[16];
static unsigned short servoPinConf = 0;

static byte runAnimate = 0;
static byte runDelay = 0;
static byte runStep = 0;
static bool reportProgPos = 0;

#if !defined(__NO_SERVO__)
static const int SERVO_NUM = 12;
myServo servo[SERVO_NUM];
#endif

static unsigned short progBufLen = 0;
static char *progbuf, *prog;

// SoftwareSerial ser(8, 9);
SoftwareSerial* sser;
Stream *ser;

//////////////////////////////////////////////////////////////////////////////
// Setup and loop functions
//////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
void iArduinoSetup(char *pbuf, unsigned short len)
{

  if (EEPROM.read(SERIAL_PORT_FLAG) == '0') {
     if (sser) {
      delete sser;
     }
     sser = new SoftwareSerial(EEPROM.read(SERIAL_PORT_RX), EEPROM.read(SERIAL_PORT_TX));
     ser = sser;
     // sser->begin(9600);
     sser->begin(38400);
   } else {
     ser = &Serial;
     Serial.begin(115200);
   }

  // while (!ser)
    // ;  // For Leonard
  
  ser->println(F("Tiny C language interpreter for Arduino"));
  ser->println(F("(c) 2012 N.M.; Updated (2014) by Guy Zyskind"));

  prog = progbuf = pbuf;
  progBufLen = len;

  if (EEPROM.read(0x3ff) == 'P') {
    ReadProg();
    if (EEPROM.read(0x3fe) == 'R') {
      int err;

      ser->println(F("Starting program"));
      runStep = 0;
      runAnimate = 0;
      runDelay = 0;
      if ((err = run(prog, strlen(prog))) != 0)
  printError(err);
    }
  }
  ser->println(F("OK"));
}
#endif

void iArduinoLoop()
{
  int err, v;
  char buf[INPUT_LEN+1];
  char *p;

  getsS(buf, INPUT_LEN);
  if (strchr(buf, 0x3) != NULL) /* Ctrl-C */
    return;
  p = buf + strlen(buf) -1;
  *p = 0; /* remove \r or \n */
  p = skipSpace(buf, p);
  if (*p == 0)
    return;
  if (strncmp(buf, "animate", 7) == 0) {
    runStep = 0;
    runAnimate = 1;
    runDelay = 500;
    if ((err = run(prog, strlen(prog))) != 0)
      printError(err);
  } else if (strncmp(buf, "autorun", 7) == 0) {
#ifdef ARDUINO
    EEPROM.write(0x3fe, 'R');
#endif
  } else if (strncmp(buf, "debug", 5) == 0) {
    runStep = 0;
    runAnimate = 1;
    runDelay = 0;
    if ((err = run(prog, strlen(prog))) != 0)
      printError(err);
  } else if (strncmp(buf, "edit", 4) == 0) {
    int l = atoi(buf+4);
    getsS(buf, INPUT_LEN);
    edit(prog, l, buf);
  } else if (strncmp(buf, "list", 4) == 0) {
    list(prog);
  } else if (strncmp(buf, "noauto", 6) == 0) {
#ifdef ARDUINO
    EEPROM.write(0x3fe, 0);
#endif
  }  else if (strncmp(buf, "prog", 4) == 0) {
    int i;
    char *p;
    prog = NULL;
    p = progbuf;
    for (i=0; i<progBufLen; i = p - progbuf) {
      //fprintf(stderr, "%d: %d\n", i, progBufLen);
      getsS(buf, INPUT_LEN);
      if (strncmp(buf, "end", 3) == 0) {
  *p = 0;
  prog = progbuf;
  break;
      }
      if (p-progbuf + strlen(buf) > progBufLen-1) {
  break;
      }
      strcpy(p, buf);
      p = progbuf + strlen(progbuf);
    }
    if (prog == NULL) {
      printSln("Program is too large");
      progbuf[0] = 0;
      prog = progbuf;
    }
  } else if (strncmp(buf, "run", 3) == 0) {
    runStep = 0;
    runAnimate = 0;
    runDelay = 0;
    if ((err = run(prog, strlen(prog))) != 0)
      printError(err);
  } else if (strncmp(buf, "port", 4) == 0) {
    // printSln("Setting Software Serial port ...");
    // EEPROM.write(SERIAL_PORT_TX, buf[5]); TODO : dynamic ...
    int a=-1, b=-1;
    char port = '9';
    sscanf(buf, "port(%c,%d,%d)", &port, &a, &b);
    if (port == '9' || a == -1 || b == -1) {
      printError(ERROR_SYNTAX);
    } else {
      EEPROM.write(SERIAL_PORT_FLAG, port);
      EEPROM.write(SERIAL_PORT_RX, a);
      EEPROM.write(SERIAL_PORT_TX, b);
      printSln("Port changed.");
    }
  } else if (strncmp(buf, "step", 4) == 0) {
    runStep = 1;
    if ((err = run(prog, strlen(prog))) != 0)
      printError(err);
  } else if (strncmp(buf, "save", 4) == 0) {
#ifdef ARDUINO
    SaveProg();
#endif
  } else if (strncmp(buf, "go", 2) == 0) {
#ifdef ARDUINO
    SaveProg();
    EEPROM.write(0x3fe, 'R');
    asm volatile ("  jmp 0");
#endif
  } else {
    char *p = strchr(buf, ';');
    if (p != NULL)
      *p = 0;
    if ((err = eval2(buf, strlen(buf), &v)) != 0)
      printError(err);
    else
      print(v);
  }
  printSln("OK");
}


//////////////////////////////////////////////////////////////////////////////
// Test and wrapper functions
//////////////////////////////////////////////////////////////////////////////
#ifndef ARDUINO
// Test functions
int rand_(int a)
{
  return rand();
}

int plus(int a, int b)
{
  return a+b;
}

int print(int a)
{
  printf("%d\n", a);
  return a;
}
#else // ARDUINO //
// Wrapper functions
#if !defined(__NO_SERVO__)
void myServo::attach(int pin)
{
  int idx = (this - servo)/sizeof(servo[0]);

  //  pin = pin;
  pwmPinConf &= ~(1<<pin);
  servoPins[pin] = idx;
  servoPinConf |= (1<<pin);
  s.attach(pin);
}

void myServo::detach()
{
  s.detach();
}

void myServo::write(int angle)
{
  int idx = (this - servo)/sizeof(servo[0]);

  servoValues[idx] = angle;
  s.write(angle);
}
#endif

int rand(int)
{
    return random(0, 32767);
}

int analogRead_(uint8_t pin)
{
  if (pin>=14)
    pin -=14;
  analogPinConf &= ~(1<<pin);
  return analogRead(pin);
}

int delay_(int ms)
{
  unsigned long st = millis();

  while ((millis() - st)<ms) {
    if (ser->available() && ser->peek() == IAR_STX) {
      ser->read(); // throw away IAR_STX
      HandleProtocol();
    }
  }
  return 0;
}

int digitalRead_(int pin)
{
   pwmPinConf &= ~(1<<pin);
   return digitalRead(pin);
}

int plus(int a, int b)
{
  return a+b;
}

int analogWrite_(int pin, int val)
{
   if (servoPinConf & (1<<pin)) {
     servo[servoPins[pin]].detach();
     servoPinConf &= ~(1<<pin);
   }
   analogWrite(pin, val); 
   pwmPinConf |= (1<<pin);
   pwmValues[pin] = val;
   return 0;
}

int digitalWrite_(int pin, int val)
{
   if (servoPinConf & (1<<pin)) {
     servo[servoPins[pin]].detach();
     servoPinConf &= ~(1<<pin);
   }
   digitalWrite(pin, val); 
   pwmPinConf &= ~(1<<pin);
   return 0;
}

int millis_(int dummy)
{
   return millis();
}

int min_(int a, int b)
{
   return min(a, b);  
}

int max_(int a, int b)
{
   return max(a, b);  
}

int noTone_(int pin)
{
  noTone(pin);
  return 0;
}

int pinMode_(int pin, int mode)
{
   pwmPinConf &= ~(1<<pin);
   if (servoPinConf & (1<<pin)) {
     servo[servoPins[pin]].detach();
     servoPinConf &= ~(1<<pin);
   }

   if (mode == 2) {
    pinMode(pin, INPUT);
#ifndef ARDUINO_LEONARD
    // Atmega168/328
    analogRead_(pin-14);
#else // Leonard (AtMega32U4)
    if (pin>=14 && pin<=19)
      analogRead_(pin-14);
    else if (pin == 4)
      analogRead_(6);   // A6
    else if (pin == 6)
      analogRead_(7);
    else if (pin == 8)
      analogRead_(8);
    else if (pin == 9)
      analogRead_(9);
    else if (pin == 10)
      analogRead_(10);
    else if (pin == 12)
      analogRead_(11);
#endif
   } else {
#ifndef ARDUINO_LEONARD
    // Atmega168/328
    if (mode == 0)
      pinMode(pin, INPUT);
    else if (mode == 1)
      pinMode(pin, OUTPUT);
    if (pin>=14 && pin<=19)
      analogPinConf |= (1<<(pin-14));
#else // Leonard (AtMega32U4)
    if (mode == 0) {
      if (pin<=13)
  pinMode(pin, INPUT);
      else if (pin<=23)
  pinMode(pin+4, INPUT); // A0-A5 は 18-23
    } else if (mode == 1) {
      if (pin<=13)
  pinMode(pin, OUTPUT);
      else if (pin<=19)         // [14, 19]
  pinMode(pin+4, OUTPUT); // A0-A5 は 18-23
    }
    if (pin>=14 && pin<=19)
      analogPinConf |= (1<<(pin-14));
    else {
      switch (pin) {
      case 4:  analogPinConf |= (1<<6); break;
      case 6:  analogPinConf |= (1<<7); break;
      case 8:  analogPinConf |= (1<<8); break;
      case 9:  analogPinConf |= (1<<9); break;
      case 10: analogPinConf |= (1<<10); break;
      case 12: analogPinConf |= (1<<11); break;
      }
    }
#endif    
  }
   return 0;
}

int print(int a)
{
  ser->println(a);
  return a;
}

int tone2(int pin, int freq)
{
  tone(pin, freq);
  return 0;
}

int tone3(int pin, int freq, int duration)
{
  tone(pin, freq, duration);
  return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////
// ser port related fucntions
///////////////////////////////////////////////////////////////////////////
#ifndef ARDUINO
void getsS(char *buf, size_t len)
{
  fgets(buf, len, stdin);
  buf[len+1] = 0;
}
void printSln(const char *s)
{
  fprintf(stderr, "%s\n", s);
}
void printS(const char *s)
{
  fprintf(stderr, "%s", s);
}
void putcS(char c)
{
  fputc(c, stderr);
}
void writeS(const char *s, int len)
{
  fwrite(s, 1, len, stderr);
}
#else
int kbhit()
{
  return ser->available();
}
int getch()
{
  return ser->read();
}
void Sleep(int s)
{
   delay(s); 
}
byte getcharS()
{
  byte c;
  while (ser->available() == 0)
    ;
  c = ser->read();
  ser->write(c);
  return c;
}
void getsS(char *buf, size_t len)
{
  static char prev = 0;
  char c, *p;
  
  p = buf;
  do {
    if (ser->available() > 0) {
      c = ser->read();
      if (c == IAR_STX) {
  HandleProtocol();
  continue;
      }
      if (p == buf && c == '\n' && prev == '\r') {
  prev = c = 0;
  continue;
      }
      if (c == 8 /* backspace*/) {
  p --;
  ser->write(8);
  ser->write(' ');
  ser->write(8);
  if (p<buf)
    p = buf;
      } else if (c == 3 /* Ctrl-C */) {
        *p = c;
        p ++;
        ser->println(F("^C"));
      } else {
        if (c == '\n')
     ser->write('\r');
  ser->write(c);
  *p = c;
  p ++;
      }
    }
  } while (c != '\n' && c != '\r' && c != 0x3 && (p-buf)<len);
  prev = c;
  if (c == '\r') {
    *(p-1) = '\n';
    ser->write('\n');
  }/* else if (c == '\n') {
    ser->write('\r');
  }*/
  *p = 0;
}
void printS(const char *s)
{
  ser->print(s); 
}
void printS(const uint8_t *s)
{
  ser->print((const char *)s);
}
void printSln(const char *s)
{
  ser->println(s); 
}
void printSln(const uint8_t *s)
{
  ser->println((const char *)s);
}
void putcS(char c)
{
  ser->write(c);
}
void writeS(const char *s, int len)
{
  ser->write((uint8_t *)s, len);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// EEPROM related functions
//////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
void ReadProg()
{
  int i;
  for (i=0; i<(progBufLen-1); i++) {
    progbuf[i] = EEPROM.read(i);
    if (progbuf[i] == 0)
      break;
  }
  if (progbuf[i] != 0)
    progbuf[i] = 0;
}
void SaveProg()
{
  int len = strlen(prog);

  for (int i=0; i<=len; i++) {
    EEPROM.write(i, prog[i]);
  }
  EEPROM.write(0x3ff, 'P'); // Magic number
}
#endif

//////////////////////////////////////////////////////////////////////////////
// iArduino language core functions
//////////////////////////////////////////////////////////////////////////////
int stepRun()
{
  char buf[INPUT_LEN+1], *p;
  int err, v;

  if (runStep == 0) {
    char c;

    if (runDelay>0)
      Sleep(runDelay);
    if (kbhit() == 0)
      return 0;
    c = getch();
    if (c == IAR_STX) {
      HandleProtocol();
      return 0;
    } else if (c != 3) {/* ! Ctrl-c */
      printSln("Paused");
    } else {
      printSln("^C");
      return STOPPED;
    }
    runStep = 1;
  }
  for (;;) {
    putcS('>');
    getsS(buf, INPUT_LEN);
    if (buf[0] == 3)
      return STOPPED;
    p = buf + strlen(buf) -1;
    *p = 0; /* remove \r or \n */

    if (buf[0] == 0)
      break;
    if (strcmp(buf, "a") == 0 || strcmp(buf, "animate") == 0) {
      runStep = 0;
      runAnimate = 1;
      runDelay = 500;
      break;
    } else if (strcmp(buf, "c") == 0 || strcmp(buf, "continue") == 0) {
      runStep = 0;
      break;
    } else if (strcmp(buf, "d") == 0 || strcmp(buf, "debug") == 0) {
      runStep = 0;
      runAnimate = 1;
      runDelay = 0;
      break;
    } else if (strcmp(buf, "S") == 0) {
      return STOPPED;
    } else if (strcmp(buf, "r") == 0 || strcmp(buf, "run") == 0) {
      runStep = 0;
      runAnimate = 0;
      runDelay = 0;
      break;
    } else {
      p = skipSpace(buf, p);
      if ((err = eval2(p, strlen(p), &v)) != 0)
  printError(err);
      else
  print(v);
    }
  }
  return 0;
}

void printError(int err)
{
  switch (err)
    {
    case ERROR_DIV0:
#ifdef ARDUINO
      ser->println(F("Divided by 0"));
#else
      printSln("Divided by 0");
#endif
      break;
    case ERROR_SYNTAX:
#ifdef ARDUINO
      ser->println(F("Syntax error"));
#else
      printSln("Syntax error");
#endif
      break;
    case STOPPED:
#ifdef ARDUINO
      ser->println(F("Stopped"));
#else
      printSln("Stopped");
#endif
      break;
    case ERROR_INTERNAL:
    default:
#ifdef ARDUINO
      ser->println(F("Syntax error"));
#else
      printSln("Syntax error");
#endif
      //printSln("Intenal error");
      break;
    }
}

int
eval2(char *s, int len, int *val)
{
  int i, l;
  char *e;
  int v, varn = -1, err;

  //fwrite(s, 1, len, stderr);
  //fprintf(stderr, "\n");

  e = s + len;
  // Skip space
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
    s ++;

  if (*s == 0 || s == e) {
    *val = 1;
    return 0;
  }

  if (*s == '(') {
    // ( で始まる場合
    char *p;
    int in = 1;

    s ++;
    while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
      s ++;
    if (*s == 0 || *s == ')')
      return ERROR_SYNTAX;

    p = s;
    while (*s != 0 && s<e) {
      if (*s == '(') {
  in ++;
      } else if (*s == ')') {
  in --;
  if (in == 0) {
    s ++;
    break;
  }
      }
      s ++;
    }
    if (in > 0)
      return ERROR_SYNTAX;
    if ((err = eval2(p, s-p-1, &v)) != 0)
      return err;
  } else if ((*s == '!' && *(s+1) != '=') || *s == '~' || *s == '-' || *s == '+') {
    // トークンが1項演算子の場合
    char *p, op;
    int in = 0;
    op = *s;
    s ++;
    while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
      s ++;
    if (*s == 0 || s == e)
      return ERROR_SYNTAX;
    p = s;
    while (*s != 0 && s<e && (in>0 || (*s != '+' && *s != '-' && *s != '>' && *s != '<' && *s != '='))) {
      if (*s == '(') {
  in ++;
      } else if (*s == ')') {
  in --;
      }
      s ++;
    }

    if (eval2(p, s-p, &v) != 0)
      return ERROR_SYNTAX;
    if (op == '!')
      v = !v;
    else if (op == '~')
      v = ~v;
    else if (op == '-')
      v = -v;
  } else if (*s>='0' && *s<='9') {
    // トークンが数値の場合
    if (*s == '0' && *(s+1) == 'x') {
      v = 0;
      s += 2;
      while (*s != 0 && s<e && ((*s>='0' && *s<='9') || 
        (*s>='a' && *s<='f') || (*s>='A' && *s<='F'))) {
  if (*s>='0' && *s<='9')
    v = (v<<4) + (*s - '0');
  else if (*s>='a' && *s<='f')
    v = (v<<4) + (*s - 'a' + 0xa);
  else if (*s>='A' && *s<='F')
    v = (v<<4) + (*s - 'A' + 0xa);
  s ++;
      }
    } else if (*s == '0' && *(s+1) == 'b') {
      v = 0;
      s += 2;
      while (*s != 0 && s<e && (*s>='0' && *s<='1')) {
  v = (v<<1) + (*s - '0');
  s ++;
      }
    } else {
      v = 0;
      while (*s != 0 && s<e && *s>='0' && *s<='9') {
  v = v*10 + (*s - '0');
  s ++;
      }
    }
  } else if (((*s>='a' && *s<='z') || (*s>='A' && *s<='Z')) &&
       (!(*(s+1)>='a' && *(s+1)<='z') && !(*(s+1)>='A' && *(s+1)<='Z')))  {
      // The token is a varibale
    if (*s>='a' && *s<='z')
      varn = *s - 'a';
    else 
      varn = *s - 'A';
    v = variables[varn];
    s ++;
  } else {
    // 定数かどうかをチェックする
    for (i=0; i<CONST_NUM; i++) {
      const struct ArduinoConstValues *c = constants+i;
      if (e-s>=c->len && strncmp(c->name, s, 3) == 0) {
  v = c->val;
  s += c->len;
  break;
      }
    }
    // Check if the token is a supported function
    // 引数が1つの関数
    for (i=0; i<FUNC1_NUM; i++) {
      const struct Function1 *f = func1+i;
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
  char *p;
  int in = 1, v1;
  s += f->len;
  p = s;
  while (*s != 0 && s<e && in>0) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  if (in>0)
    return ERROR_SYNTAX;
  if (eval2(p, s-p-1, &v1) != 0)
    return ERROR_SYNTAX;
  
  v = (*f->func)(v1);
      }
    }

#if !defined(__NO_SERVO__)
    // servo?.attach(), servo?.write()
    for (i=0; i<SERVO_NUM; i++) {
      char f[20];
      int attach_write = 0, len;
      sprintf(f, "servo%d.attach(", i);
      len = strlen(f);
      if (e-s>=len && strncmp(f, s, len) == 0) {
        attach_write = 1;
      } else {
        sprintf(f, "servo%d.write(", i);
        len = strlen(f);
        if (e-s>=len && strncmp(f, s, len) == 0) {
          attach_write = 2;
        } else {
    sprintf(f, "servo%d.detach(", i);
    len = strlen(f);
    if (e-s>=len && strncmp(f, s, len) == 0) {
      attach_write = 3;
    }
  }
      }
      if (attach_write > 0) {
  char *p;
  int in = 1, v1;
  s += len;
  p = s;
  while (*s != 0 && s<e && in>0) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  if (in>0)
    return ERROR_SYNTAX;
  if (eval2(p, s-p-1, &v1) != 0)
    return ERROR_SYNTAX;

        if (attach_write == 1) {
          servo[i].attach(v1);
        } else if (attach_write == 2) {
          servo[i].write(v1);
  } else {
          servo[i].detach();
  }
        v = 0;
        break;
      }
    }
#endif

    // 引数が2つの関数
    for (i=0; i<FUNC2_NUM; i++) {
      const struct Function2 *f = func2+i;
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
  char *p;
  int in = 0, v1, v2;
  s += f->len;
  p = s;
  while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
    if (*s == '(')
      in ++;
    else if (*s == ')')
      in --;
    s ++;
  }
  if (*s != ',')
    return ERROR_SYNTAX;
  if (eval2(p, s-p, &v1) != 0)
    return ERROR_SYNTAX;

  s ++;
  p = s;
  in = 1;
  while (*s != 0 && s<e && in>0) {
    if (*s == '(')
      in ++;
    else if (*s == ')')
      in --;
    s ++;
  }
  if (in>0)
    return ERROR_SYNTAX;
  if (eval2(p, s-p-1, &v2) != 0)
    return ERROR_SYNTAX;

  v = (*f->func)(v1, v2);
        break;
      }
    }

    // 引数が3つの関数
    for (i=0; i<FUNC3_NUM; i++) {
      const struct Function3 *f = func3+i;
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
  char *p;
  int in = 0, v1, v2, v3;
  s += f->len;
  p = s;
  while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
    if (*s == '(')
      in ++;
    else if (*s == ')')
      in --;
    s ++;
  }
  if (*s != ',')
    return ERROR_SYNTAX;
  if (eval2(p, s-p, &v1) != 0)
    return ERROR_SYNTAX;

  s ++;
  p = s;
  in = 0;
  while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
    if (*s == '(')
      in ++;
    else if (*s == ')')
      in --;
    s ++;
  }
  if (*s != ',')
    return ERROR_SYNTAX;
  if (eval2(p, s-p-1, &v2) != 0)
    return ERROR_SYNTAX;

  s ++;
  p = s;
  in = 1;
  while (*s != 0 && s<e && in>0) {
    if (*s == '(')
      in ++;
    else if (*s == ')')
      in --;
    s ++;
  }
  if (in>0)
    return ERROR_SYNTAX;
  if (eval2(p, s-p-1, &v3) != 0)
    return ERROR_SYNTAX;

  v = (*f->func)(v1, v2, v3);
        break;
      }
    }
  }

  // 次のトークンを確かめる
    {
      int v2;

      // Skip space
      while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
  s ++;

      if (*s == 0 || s == e) { // 後ろに演算子はなかった
  *val = v;
  return 0;
      }

      if (*s == '=' && *(s+1) != '=') {
  char *p;
  int op = *s, in = 0;

  s ++;
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
    s ++;
  if (*s == 0 || s == e)
    return ERROR_SYNTAX;

  p = s;
  while (*s != 0 && s<e && (in>0 || 
          !(*s == '=' && *(s+1) == '='))) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  
  if ((err = eval2(p, s-p, &v2)) != 0)
    return err;

  if (varn < 0 || varn>=sizeof(variables))
    return ERROR_INTERNAL;
  v = variables[varn] = v2;

  if (*s == 0 || s == e) {
    *val = v;
    return 0;
  }
      }


      while (*s == '*' || *s == '/' || *s == '%') {
  char *p;
  int op = *s, in = 0;

  s ++;
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
    s ++;
  if (*s == 0 || s == e)
    return ERROR_SYNTAX;

  p = s;
  while (*s != 0 && s<e && (in>0 || 
         (*s != '*' && *s != '/' && *s != '%' &&
          *s != '+' && *s != '-' && *s != '>' && *s != '<' &&
          !(*s == '=' && *(s+1) == '=')  && *s != '!'))) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  
  if ((err = eval2(p, s-p, &v2)) != 0)
    return err;
  switch (op) {
  case '*':
    v = v*v2;
    break;
  case '/':
    v = v/v2;
    break;
  case '%':
    v = v%v2;
    break;
  }
  if (*s == 0 || s == e) {
    *val = v;
    return 0;
  }
      }
    
      if (*s == '+' || *s == '-') {
  char *p;
  int op = *s, in = 0;

  p = s;
  while (*s != 0 && s<e && (in>0 || (*s != '>' && *s != '<' && *s != '=' && *s != '!'))) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  if (in != 0)
    return ERROR_SYNTAX;

  // 演算子の右側を計算する
  if ((err = eval2(p, s-p, &v2)) != 0)
    return err;
  // 左と右を足す
  v = v + v2;
  if (*s == 0 || s==e) {
    *val = v;
    return 0;
  }
      }

      if (*s == '>' || *s == '<' || (*s == '=' && *(s+1) == '=')
    || (*s == '!' && *(s+1) == '=')) {
  char *p;
  int op = *s, op2 = *(s+1), in = 0;

  s ++;
  if (op2 == '=' || op2 == '<' || op2 == '>')
    s ++;

  p = s;
  while (*s != 0 && s<e && (in>0 || (*s != '&' && *s != '^' && *s != '|'))) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  if (in != 0)
    return ERROR_SYNTAX;

  // 演算子の右側を計算する
  if ((err = eval2(p, s-p, &v2)) != 0)
    return err;

  if (op == '>') {
      if (op2 == '=') {
        v = v>=v2;
      } else if (op2 == '>') {
        v = v>>v2;
      } else {
        v = v>v2;
      }
  } else if (op == '<') {
    if (op2 == '=') {
      v = v<=v2;
    } else if (op2 == '<') {
      v = v<<v2;
    } else {
      v = v<v2;
    }
  } else if (op == '=' && op2 == '=') {
    v = (v == v2);
  } else if (op == '!' && op2 == '=') {
    v = (v != v2);
  }
  if (*s == 0 || s==e) {
    *val = v;
    return 0;
  }
      }

      if ((*s == '&' && *(s+1) != '&') || (*s == '|' && *(s+1) != '|') ||
     *s == '^')  {
  char *p;
  int op = *s, in = 0;

  s ++;
  p = s;
  while (*s != 0 && s<e && (in>0 || !((*s == '&' && *(s+1) == '&') ||
              (*s == '|' && *(s+1) == '|')))) {
    if (*s == '(') {
      in ++;
    } else if (*s == ')') {
      in --;
    }
    s ++;
  }
  if (in != 0)
    return ERROR_SYNTAX;

  // 演算子の右側を計算する
  if ((err = eval2(p, s-p, &v2)) != 0)
    return err;
  if (op == '&')
    v = v & v2;
  else if (op == '|')
    v = v | v2;
  else if (op == '^')
    v = v ^ v2;
  if (*s == 0 || s==e) {
    *val = v;
    return 0;
  }
      }

      if ((*s == '&' && *(s+1) == '&') || (*s == '|' && *(s+1) == '|')) {
  int op = *s;

  s += 2;
  // 演算子の右側を計算する
  if ((err = eval2(s, e-s, &v2)) != 0)
    return err;
  if (op == '&')
    v = v && v2;
  else if (op == '|')
    v = v || v2;

  *val = v;
  return 0;
      }

    }
error:
  return ERROR_INTERNAL;
}

char *skipSpace(char *s, char *e)
{
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
    s ++;
  return s;
}

int
run(char *prg, int len)
{
  char *cur, *s, *e;
  int err, in, v, skip;

  s = prg;
  e = prg + len;

  for (;;) {
    char *p, *start;
    start = s = skipSpace(s, e);
    if (*s == 0 || s>=e)
      break;

    skip = 0;
    // fprintf(stderr, s);

    if ((strncmp(s, "continue", 8) == 0
   && (*(s+8) == ';' ||
       *(s+8) == ' ' || *(s+8) == '\t' || 
       *(s+8) == '\r' || *(s+8) == '\n')) ||
  (strncmp(s, "break", 5) == 0
   && (*(s+5) == ';' ||
       *(s+5) == ' ' || *(s+5) == '\t' || 
       *(s+5) == '\r' || *(s+5) == '\n'))) {
      char *cond, *cond_e, *p1, *statement, *statement_e;
      char *statement2 = NULL, *statement2_e;
      int continue_break = 0;

      p1 = s;
      if (strncmp(s, "continue",8) == 0) {
  continue_break = 1;
  s += 8;
      } else {
  s += 5;
      }
      s = skipSpace(s, e);
      sendProgPos(start-prog, s-start);
      if (*s != ';') {
  writeS(p1, 20);
  return ERROR_SYNTAX;
      }
      s ++;

      if (continue_break == 0)
  return FOUND_BREAK;
      return FOUND_CONTINUE;
    } else if ((strncmp(s, "while", 5) == 0
   && (*(s+5) == '(' ||
       *(s+5) == ' ' || *(s+5) == '\t' || 
       *(s+5) == '\r' || *(s+5) == '\n')) ||
  (strncmp(s, "if", 2) == 0
   && (*(s+2) == '(' ||
       *(s+2) == ' ' || *(s+2) == '\t' || 
       *(s+2) == '\r' || *(s+2) == '\n'))) {
      char *cond, *cond_e, *p1, *statement, *statement_e;
      char *statement2 = NULL, *statement2_e;
      int while_if = 0;
      if (strncmp(s, "while", 5) == 0) {
  while_if = 1;
  s += 5;
      } else
  s += 2;
      p1 = s;
      s = skipSpace(s, e);
      if (*s != '(') {
  writeS(p1, 20);
  sendProgPos(start-prog, 10);
  return ERROR_SYNTAX;
      }
      s ++;
      cond = s;
      in = 1;
      while (*s !=0 && s<e && (in>0 || *s != ')')) {
  if (*s == '(')
    in ++;
  if (*s == ')') {
    in --;
    if (in == 0)
      break;
  }
  s ++;
      }
      if (*s != ')') {
  writeS(p1, s-p1);
  sendProgPos(start-prog, s-start);
  return ERROR_SYNTAX;
      }
      cond_e = s;
      s ++;
      s = skipSpace(s, e);
      if (*s == 0 || s>=e) {
  writeS(p1, s-p1);
  sendProgPos(start-prog, s-start);
  return ERROR_SYNTAX;
      }

      statement = s;
      in = 0;
      while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
  if (*s == '{')
    in ++;
  else if (*s == '}') {
    in --;
    if (in == 0)
      break;
  }
  s ++;
      }
      if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
  writeS(p1, s-p1);
  sendProgPos(start-prog, s-start);
  return ERROR_SYNTAX;
      }
      s ++;
      statement_e = s;
      // 必要なら else 部分を取り出す
      s = skipSpace(s, e);
      // fprintf(stderr, "*1 %s\n", s);
      if (while_if == 0 && strncmp(s, "else", 4) == 0
    && (*(s+4) == '{' ||
        *(s+4) == ' ' || *(s+4) == '\t' || 
        *(s+4) == '\r' || *(s+4) == '\n')) {
  s += 4;
  s = skipSpace(s, e);
  if (*s == 0 || s>=e) {
    writeS(p1, s-p1);
    sendProgPos(start-prog, s-start);
    return ERROR_SYNTAX;
  }
  //fprintf(stderr, "*2 %s\n", s);
  statement2 = s;
  in = 0;
  while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
    if (*s == '{')
      in ++;
    else if (*s == '}') {
      in --;
      if (in == 0)
        break;
    }
    s ++;
  }
  if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
    writeS(p1, s-p1);
    sendProgPos(start-prog, s-start);
    return ERROR_SYNTAX;
  }
  statement2_e = s;
      }

      if (while_if>0) {
  // while 文の処理
  sendProgPos(start-prog, cond_e-start);
  if (!reportProgPos && (runAnimate || runStep)) {
    printS("while (");
    writeS(cond, cond_e-cond);
    printS(")\n");
  }
  if (!reportProgPos && (err = stepRun()) != 0)
    return err;
  for (;;) {
    err = eval2(cond, cond_e-cond, &v);
    if (err)
      return err;
    if (!reportProgPos && (runAnimate || runStep)) {
      writeS(cond, cond_e-cond);
      printS(": ");
      printS(v ? "true" : "false");
      printSln("");
    }
    //fwrite(cond, 1, cond_e-cond, stderr);
    //fprintf(stderr, ": %d\n", v);
    if (v == 0)
      break;
    if (!reportProgPos && (runAnimate || runStep)) {
      if (err = stepRun() != 0)
        return err;
    }
    err = run(statement, statement_e-statement);
    if (err == FOUND_CONTINUE)
      continue;
    else if (err == FOUND_BREAK)
      break;
    else if (err)
      return err;
  }
      } else {
  // if 文の処理
  sendProgPos(start-prog, cond_e-start);
  if (!reportProgPos && (runAnimate || runStep)) {
    printS("if (");
    writeS(cond, cond_e-cond);
    printS(")");
  }
  err = eval2(cond, cond_e-cond, &v);
  if (err) {
    if (!reportProgPos && (runAnimate || runStep))
      printSln("");
    return err;
  }
  if (!reportProgPos && (runAnimate || runStep)) {
    printS(": ");
    printS(v ? "true" : "false");
    printSln("");
  }
  //fwrite(cond, 1, cond_e-cond, stderr);
  //fprintf(stderr, ": %d\n", v);
  if (v != 0) {
    err = run(statement, statement_e-statement);
    if (err)
      return err;
  } else {
    if (statement2 != NULL) {
      err = run(statement2, statement2_e-statement2);
      if (err)
        return err;
    }
  }
      }
    } else if ((strncmp(s, "for", 3) == 0
   && (*(s+3) == '(' ||
       *(s+3) == ' ' || *(s+3) == '\t' || 
       *(s+3) == '\r' || *(s+3) == '\n'))) {
      char *init, *init_e;
      char *cond, *cond_e, *inc, *inc_e;
      char *statement, *statement_e, *p1;
      p1 = s;
      s += 3;
      s = skipSpace(s, e);
      if (*s != '(') {
  writeS(p1, 20);
  return ERROR_SYNTAX;
      }
      s ++;
      s = skipSpace(s, e);
      init = s;
      while (*s !=0 && s<e && *s != ';')
  s ++;
      if (*s == 0 || s>=e) {
  writeS(p1, 20);
  return ERROR_SYNTAX;
      }
      init_e = s;
      s ++;
      s = skipSpace(s, e);
      cond = s;
      while (*s !=0 && s<e && *s != ';')
  s ++;
      if (*s == 0 || s>=e) {
  writeS(p1, s-p1);
  return ERROR_SYNTAX;
      }
      cond_e = s;
      s ++;
      inc = s = skipSpace(s, e);
      in = 1;
      while (*s !=0 && s<e && (in>0 || *s != ')')) {
  if (*s == '(')
    in ++;
  else if (*s == ')') {
    in --;
    if (in == 0)
      break;
  }
  s ++;
      }
      if (*s == 0 || s>=e) {
  writeS(p1, s-p1);
  return ERROR_SYNTAX;
      }
      inc_e = s;
      s ++;
      s = skipSpace(s, e);
      statement = s;
      in = 0;
      while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
  if (*s == '{')
    in ++;
  else if (*s == '}') {
    in --;
    if (in == 0)
      break;
  }
  s ++;
      }
      if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
  writeS(p1, s-p1);
  return ERROR_SYNTAX;
      }
      s ++;
      statement_e = s;

      // for 文の処理
      if (reportProgPos)
  sendProgPos(start-prog, inc_e-start);
      else if (runAnimate || runStep) {
  printS("for (");
  writeS(init, init_e-init);
  printS("; ");
  writeS(cond, cond_e-cond);
  printS("; ");
  writeS(inc, inc_e-inc);
  printS(")\n");
      }
      if (!reportProgPos && (runAnimate || runStep)) {
  if ((err = stepRun()) != 0)
    return err;
  writeS(init, init_e-init);
  printSln("");
      }
      if ((err = eval2(init, init_e-init, &v)) != 0) {
  return err;
      }
      if (!reportProgPos && (runAnimate || runStep))
  if(err = stepRun() != 0)
    return err;
      for (;;) {
  sendProgPos(cond-prog, cond_e-cond+1);
  if ((err = eval2(cond, cond_e-cond, &v)) != 0) 
    return err;
  if (!reportProgPos && (runAnimate || runStep)) {
    writeS(cond, cond_e-cond);
    printS(": ");
    printS(v ? "true" : "false");
    printSln("");
  }
  if (v == 0)
    break;
  //fwrite(statement, 1, statement_e-statement, stderr);
  if ((err = run(statement, statement_e-statement)) != 0) {
    if (err == FOUND_CONTINUE)
      ; /* DO_NOTHING */
    else if (err == FOUND_BREAK)
      break;
    else return err;
  }
  sendProgPos(inc-prog, inc_e-inc+1);
  if (!reportProgPos && (runAnimate || runStep)) {
    writeS(inc, inc_e-inc);
    printSln("");
  }
  if ((err = eval2(inc, inc_e-inc, &v)) != 0)
    return err;
  if (runAnimate || runStep) {
    if ((err = stepRun()) != 0)
      return err;
  }
      }
    } else if (*s == '{' || *s == '}') {
      s ++;
      skip = 1;
    } else { // 式文の処理
      p = s;
      in = 0;
      while (*s != 0 && s<e && (in>0 || *s != ';'))
  s ++;
      if (*s == 0 && in>0) {
  writeS(p, s-p);
  return ERROR_SYNTAX;
      }
      if (s != p) {
  if (!reportProgPos && (runAnimate != 0 || runStep)) {
    writeS(p, s-p);
    printSln("");
  }
  sendProgPos(p-prog, s-p);
  if ((err = eval2(p, s-p, &v)) != 0) {
    writeS(p, s-p);
    printSln("");
    return err;
  }
      } else
  skip = 1;
      s ++;
#if 0
      printf("%d\n", v);
#endif
      if (skip == 0) {
  if ((err = stepRun()) != 0)
    return err;
      }
    }
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////
// iArduino shell related functions
///////////////////////////////////////////////////////////////////////////
void edit(char *prog, int lNum, char *nLine)
{
  char *p, *q;
  int editLine;
  int l;

#if 0
  // Get line number to edit
  lNum = 0;
  p = nLine;
  while (*p != ' ') {
    if (*p>='0' && *p<='9')
      lNum = lNum*10 + (*p - '0');
    else {
      printSln("Invalid line number");
      return;
    }
    p ++;
  }
  nLine = p + 1;
#endif

  p = q = prog;
  l = 1;
  while (*q && l<=lNum)
  {
     if (*q == '\r' || *q =='\n') {
        if (*q == '\r' && *(q+1) == '\n')
          q ++;
        q ++;
  if (l == lNum || *q == 0)
    break;
  l ++;
        p = q;
        continue;
     }
     q ++;
  }
  // Append given line to the program if the line number is larger than current list.
  if (lNum>l && *q == 0) {
    strcat(prog, nLine);
    return;
  }
  // Replace the line with given one
  if ((q - p)<strlen(nLine)) {
    int sz = strlen(nLine) - (q-p);
    char *pp = prog + strlen(prog) + 1;
    char *qq = pp + sz;
    for (int i=strlen(q)+1; i>=0; i--) {
      *qq = *pp;
      qq --; pp --;
    }
    q = p + strlen(nLine);
  }
  for (int i=strlen(nLine); i>0; i--) {
    *p = *nLine;
    p ++; nLine ++;
  }
  if (p == q)
    return;
  for (int i=strlen(q); i>0; i--) {
    *p = *q;
    p ++; q ++;
  }
  *p = 0;
}

void list(char *prog)
{
  char *p, *q;
  int l = 1;
  
  p = q = prog;
  while (*q)
  {
     if (*q == '\r' || *q =='\n') {
#ifdef ARDUINO
       ser->print(l);
       ser->print(F(" "));
#else
       printf("%d ", l);
#endif
        writeS(p, q-p);
        printSln("");
  l ++;
        if (*q == '\r' && *(q+1) == '\n')
          q ++;
        q ++;
        p = q;
        continue;
     }
     q ++;
  }
}

///////////////////////////////////////////////////////////////////////////
// Main function for PC and UNIX
///////////////////////////////////////////////////////////////////////////
#ifndef ARDUINO
int
main(int argc, char **argv)
{
  int v;

  if (argc == 2) {
    if (eval2(argv[1], strlen(argv[1]), &v) != 0)
      printf("error\n");
    print(v);
  } else {
    prog = progbuf;
    for (;;)
      loop();
  }

  return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
// iArduinoTeminal related functions
//////////////////////////////////////////////////////////////////////////////
void HandleProtocol()
{
  byte pin, out, idx;
  unsigned long current;
  int c;

  while (!ser->available()) {}
  c = ser->read();

  switch (c) {
  case IAR_REPORT_PROTOVER:
    ser->write((unsigned char)IAR_STX);
    ser->write((unsigned char)IAR_REPORT_PROTOVER);
#ifndef ARDUINO_LEONARD
    ser->write((unsigned char)IAR_PROTOVER_MEGA328v1);
#else
    ser->write((unsigned char)IAR_PROTOVER_LEONARDv1);
#endif
    break;
  case IAR_REPORT_VARIABLES:
    ser->write((unsigned char)IAR_STX);
    ser->write((unsigned char)IAR_REPORT_VARIABLES);
    for (int i=0; i<26; i++) {
      ser->write((unsigned char)((variables[i] & 0xff00) >> 8));
      ser->write((unsigned char)(variables[i] & 0xff));
    }
    break;
  case 0x2:
    ser->write(IAR_STX);
    ser->write(0x2);
#ifndef ARDUINO_LEONARD
    // digital pins 0 to 7
    ser->write(DDRD);  // data direction register (R)
    ser->write(PORTD); // data register       (R/W)
    ser->write(PIND);  // input pins register (R)
    // digital pins 8 to 13 (14 and 15 are crystal pins)
    ser->write(DDRB);  // data direction register (R)
    ser->write(PORTB); // data register       (R/W)
    ser->write(PINB);  // input pins register (R)
    // analog pins 0 to 5 (and 6 to 7)
    ser->write(DDRC);  // data direction register (R)
    ser->write(PORTC); // data register       (R/W)
    ser->write(PINC);  // input pins register (R)
#else
    ser->write(DDRD);  // data direction register (R)
    ser->write(PORTD); // data register       (R/W)
    ser->write(PIND);  // input pins register (R)

    ser->write(DDRF);  // data direction register (R)
    ser->write(PORTF); // data register       (R/W)
    ser->write(PINF);  // input pins register (R)

    ser->write((DDRB & 0xf0)  | ((DDRC  & 0xc0) >> 4) | (DDRE  & 0xc0) >> 6);
    ser->write((PORTB & 0xf0) | ((PORTC & 0xc0) >> 4) | (PORTE & 0xc0) >> 6);
    ser->write((PINB & 0xf0)  | ((PINC  & 0xc0) >> 4) | (PINE  & 0xc0) >> 6);
#endif
    break;
  case 0x3:
    ser->write(IAR_STX);
    ser->write(0x3);
#ifndef ARDUINO_LEONARD
    ser->write((unsigned char)(analogPinConf & 0xff));
#else
    ser->write((unsigned char)((analogPinConf & 0xff00) >> 8));
    ser->write((unsigned char)(analogPinConf & 0xff));
#endif
    for (int i=0; i<ANALOG_PINS; i++) {
      if (analogPinConf & (1<<i)) {
  ser->write((unsigned char)0);
  ser->write((unsigned char)0);
      } else {
  int a = analogRead(i);
  ser->write((unsigned char)((a & 0xff00) >> 8));
  ser->write((unsigned char)(a & 0xff));
      }
    }
    break;
  case IAR_REPORT_OTHERS:
    ser->write(IAR_STX);
    ser->write(IAR_REPORT_OTHERS);
    ser->write((unsigned char)((pwmPinConf & 0xff00) >> 8));
    ser->write((unsigned char)(pwmPinConf & 0xff));
    ser->write((unsigned char)((servoPinConf & 0xff00) >> 8));
    ser->write((unsigned char)(servoPinConf & 0xff));
    for (int i=0; i<16; i++) {
      if (pwmPinConf & (1<<i))
  ser->write(pwmValues[i]);
      else if (servoPinConf & (1<<i))
  ser->write(servoValues[servoPins[i]]);
      else
  ser->write((unsigned char)0);
    }
    break;
  case IAR_REPORT_PROG_LIST:
    ser->write(IAR_STX);
    ser->write(IAR_REPORT_PROG_LIST);
    ser->write(prog);
    ser->write(IAR_STX);
    break;
  case IAR_REPORT_PROG_POS:
    reportProgPos = true;
    break;
  case IAR_REPORT_TIME:
    current = millis();
    ser->write(IAR_STX);
    ser->write(IAR_REPORT_TIME);
    ser->write((unsigned char)(current >> 24));
    ser->write((unsigned char)((current >> 16)&0xff));
    ser->write((unsigned char)((current >> 8)&0xff));
    ser->write((unsigned char)(current & 0xff));
    break;
  case IAR_SET_PINMODE:
    while(!ser->available()) ;
    pin = ser->read();
    while(!ser->available()) ;
    out = ser->read();
    pinMode_(pin, out);
    break;
  case IAR_SET_DIGITAL:
    while(!ser->available()) ;
    pin = ser->read();
    while(!ser->available()) ;
    out = ser->read();
#ifndef ARDUINO_LEONARD
    digitalWrite(pin, out);
#else
    if (pin<=13)
      digitalWrite(pin, out);
    else
      digitalWrite(pin+4, out);
#endif
    break;
  case IAR_SET_PWM:
    while(!ser->available()) ;
    pin = ser->read();
    while(!ser->available()) ;
    out = ser->read();
    analogWrite_(pin, out);
    break;
  case IAR_SET_SERVO:
    while(!ser->available()) ;
    idx = ser->read();
    while(!ser->available()) ;
    out = ser->read();
#if !defined(__NO_SERVO__)
    servo[idx].write(out);
#endif
    break;
  case IAR_ATTACH_SERVO:
    while(!ser->available()) ;
    idx = ser->read(); // servo?
    while(!ser->available()) ;
    pin = ser->read(); // pin
#if !defined(__NO_SERVO__)
    servo[idx].attach(pin);
#endif
    break;
  default:
    break;
  }
}

void sendProgPos(unsigned short s, unsigned short len)
{
  if (!reportProgPos)
    return;

  ser->write(IAR_STX);
  ser->write(IAR_REPORT_PROG_POS);
  ser->write((unsigned char)(s>>8));
  ser->write((unsigned char)(s & 0xff));
  ser->write((unsigned char)(len>>8));
  ser->write((unsigned char)(len&0xff));
}

#ifdef ARDUINO
void iArduinoHandleProtocol::begin(bool verbose)
{
  if (EEPROM.read(SERIAL_PORT_FLAG) == '0') {
    if (sser) {
      delete sser;
     }
     sser = new SoftwareSerial(EEPROM.read(SERIAL_PORT_RX), EEPROM.read(SERIAL_PORT_TX));
     ser = sser;
     // sser->begin(9600);
     sser->begin(38400);
   } else {
     ser = &Serial;
     Serial.begin(115200);
   }

  // while (!ser)
    // ;  // For Leonard
  if (verbose)
    ser->println(F("Protocol handler for iArduino begins"));
}

void iArduinoHandleProtocol::check()
{
    if (ser->available() && ser->peek() == IAR_STX) {
      ser->read(); // throw away IAR_STX
      HandleProtocol();
    }
}

bool iArduinoHandleProtocol::available()
{
  if (ser->available()) {
    if (ser->peek() != IAR_STX)
      return true;
    else {
      ser->read(); // throw away IAR_STX
      HandleProtocol();
    }
  }
  return false;
}

int iArduinoHandleProtocol::read()
{
  int r = ser->read();

  while (r == IAR_STX) {
    HandleProtocol();
    r = ser->read();
  }
  return r;
}
#endif

/// TODO
// メモリ(RAM)の節約(中間表現を使う)
// ヒストリのサポート

// プログラムの例  //
/* // LED の点滅 (1) //
pinMode(13, OUTPUT);
for(;;) {
digitalWrite(13, HIGH);
delay(500);
digitalWrite(13, LOW);
delay(500);
}
 */
