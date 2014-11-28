// This shows how you prepare an iArduino binary for an Arduino
// by Noriaki Mitsunaga
// This example code is in the public domain.
//
// $Id: iArduino.ino,v 1.7 2012/03/08 10:52:23 kurobox Exp kurobox $
// See http://n.mtng.org/ele/arduino/iarduino.html (in English)
// See http://n.mtng.org/ele/arduino/iarduino-j.html (in Japanese)

#include <EEPROM.h>
#include  <iArduino.h>
#include <SoftwareSerial.h>
#include <Servo.h>

// プログラムを記憶する配列変数を定義し、
// 初期設定時ののプログラムを代入する
// (EEPROMに書き込むと、ここに書いたものは使われない)
// (多くても1022まで, EEPROMの最後の２バイトをマジックナンバーに使うため)

char iArduinoProgBuf[600] = "for(i=0;;i=i+1){print(i);}";

// 他の例:
// "a=1;b=1;\r\nc=1;\r\na=a+b+c;\r\nprint(a);\r\nprint(b);\r\n";
// "a=0;while(a<5)a=a+1;print(a);\r\na=5;while(a>0) {print(a);a=a-1;}\r\n";
// "if(a>b)print(10);else print(-10);";
// "if(a>b)print(10); print(-10);";
// "print(i);for(i=0;i<5;i=i+1)print(i);";
// "print(i);for(i=0;i<5;i=i+1){print(i);if(i>2)break; print(0);} print(i);\r\n";

void setup()
{
  iArduinoSetup(iArduinoProgBuf, sizeof(iArduinoProgBuf));
}

void loop()
{
  iArduinoLoop();
}
