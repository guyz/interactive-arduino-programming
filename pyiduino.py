'''
Created on Nov 26, 2014

@author: GuyZ
'''

'''
TODOs:
- support floats?
- implement -run-on-device- (1. parses python --> arduino, 2. burns the program to the board
 and resets the instruction counter to 0)
- important libs
- better exception handling
- in interpreter: fix rand, add INPUT_PULLUP
'''

import serial
import time

ERROR = -1
LOW = 'LOW'
HIGH = 'HIGH'
INPUT = 'INPUT'
OUTPUT = 'OUTPUT'
true = 'true'
false = 'false'

class Iduino(object):
    '''
    classdocs
    '''


    def __init__(self, port, speed=115200):
        self._port = port
        self._speed = speed
        self._ser = serial.Serial(port,speed)
        if self._ser.isOpen():
            self._ser.close()
        self._ser.open()

    ### Arduino methods
    
    def abs(self, n):
        try:
            return int(self.__execFunc(self.abs, n))
        except:
            return ERROR
    
    def analogRead(self, i):
        try:
            return int(self.__execFunc(self.analogRead, i))
        except:
            return ERROR
    
    def analogWrite(self, i, v):
        self.__execFunc(self.analogWrite, i, v)
    
    def delay(self, t):
        self.__execFunc(self.delay, t)
    
    def digitalRead(self, i):
        try:
            return int(self.__execFunc(self.digitalRead, i))
        except:
            return ERROR
    
    def digitalWrite(self, i, v):
        self.__execFunc(self.digitalWrite, i, v)
    
    def max(self, n, m):
        try:
            return int(self.__execFunc(self.max, n, m))
        except:
            return ERROR
    
    def millis(self):
        try:
            return long(self.__execFunc(self.millis))
        except:
            return ERROR
    
    def min(self, n, m):
        try:
            return int(self.__execFunc(self.min, n, m))
        except:
            return ERROR
    
    def noTone(self, i):
        self.__execFunc(self.noTone, i)
    
    def rand(self, *args): # TODO: needs to be fixed in the interpreter
        try:
            if (len(args) == 1):
                return int(self.__execFunc(self.rand, args[0]))
            else:
                return int(self.__execFunc(self.rand, args[0], args[1]))
        except:
            return ERROR
    
    def pinMode(self, i, v):
        self.__execFunc(self.pinMode, i, v)
    
    def tone(self, i, freq):
        self.__execFunc(self.tone, i, freq)
    
#     def print(self):
#         pass
        
#     servo?.attach, servo?.write # TODO: define as an object

    # uploads and runs a program
    def upload(self, prog):
        self._ser.flushInput()
        # c, S commands ensure the board is ready to
        # accept a program (this is a workaround)
        self._ser.write('c\r\n')
#         time.sleep(0.1)
        self._ser.write('S\r\n')
#         time.sleep(0.1)
        self._ser.write('prog\r\n')
        
        if prog.startswith('\n'): prog = prog[1:]
        if not prog.endswith('\n'): prog = prog + '\n'
        prog = prog.replace('\n','\r\n')
        
        self._ser.write(prog)
        self._ser.write('end\r\n')
        self._ser.write('save\r\n')
#         if (autorun):
#             self._ser.write('autorun\r\n')
        self._ser.write('go\r\n')
        
    
    def __execFunc(self, func, *args):
        f = '\r\n' + func.__name__ + '(' 
        for v in args:
            f = f + str(v) + ','
        f = f[:-1] + ')\r\n'
        
        self._ser.flushInput()
        self._ser.write(f)
        self._ser.readline() # echo
        return self._ser.readline().strip( "\r\n" )
            

class Servo(object):
    '''
    Wrapper for an arduino Servo object
    '''
    pinToServoMap = {}
    idCounter = 0
    def __init__(self, iduino):
        self._iduino = iduino
        
    def attach(self, i):
        if not (i in Servo.pinToServoMap):
            Servo.idCounter += 1
            Servo.pinToServoMap[i] = Servo.idCounter
        
        self._id = Servo.pinToServoMap[i]
        self.__execFunc(self.attach, i)

    def write(self, v):
        self.__execFunc(self.write, v)
                
    def __execFunc(self, func, v):
        f = '\r\n' + 'servo' + str(self._id) + '.' + func.__name__ + '(' + str(v) + ')\r\n' 
        
        self._iduino._ser.flushInput()
        self._iduino._ser.write(f)
        self._iduino._ser.readline() # echo
        return self._iduino._ser.readline().strip( "\r\n" )
        