Interactive Arduino Programming
===============================

This package allows you to interactively program an Arduino (or a similar microcontroller) board using Python. Perhaps the largest motivation of this project stems from wirelessly programming a remote board.

The main features are:
1. Supports most built-in Arduino IDE functions, ported to Python (refer to the example IPython notebook or pyiduino.py)
2. Servo library support
3. Wireless communication through any SoftwareSerial interface.
4. Upload C-like programs directly to the Arduino board. No need to go through the IDE. Note that you can still interactively program the board using the interpreter - but whenever you restart the Arduino it will start running the deployed program automatically.


## Usage

The provided examples are included in the IPython notebook. In case you're not familiar with IPython notebook, you can look into the notebook.html for the examples, and execute/modify them in an editor of your choice.

To get started, follow the steps below:

### Install the iArduino Library

Clone this repository and open the Arduino IDE. Then, click on Sketch -> Import Library -> Add Library and select the iArduino folder.

NOTE: The iArduino library is a tiny C interpreter that runs on Arduino-like boards. I modified the code to include some additional features, but the original library code can be found here: http://n.mtng.org/ele/arduino/iarduino.html.

### Upload the C-interpreter sketch.

In the Arduino IDE, click on File->Examples-iArduino->iArduino to open the interpreter sketch. Next, upload the sketch to your board.

### Programming

Now for the interesting part - let's work through the examples found in the notebook.

Load the module.

```python
from pyiduino import *
```

Select the appropriate port. If this is the first time using this module, start by connecting your board to the USB (as usual) and set the usb serial port to the Arduino.
```python
# Hardware Serial
PORT = '/dev/tty.usbserial-A900ce0g' # set this to your port
a = Iduino(PORT)
```

Essentially, you can communicate with the board using any interface that supports serial communication - but you will have to program the board to the right Software Serial pins. Here's an example that allows you to configure the Arduino so that it is programmable remotely using the low-cost HC-05 Bluetooth module. Again, this is just an example - anything would work.

```python
a.changePort(False, rxPin, txPin) # This sets HardwareSerial to false, and uses rxPin/txPin as the software serial
```

Note: you can always change the interface back to hardware serial by issuing:

```python
a.changePort(True)
```

Now restart the board, and connect to the new port (make sure you connected your module to the right rx/tx pins). E.g.,
```python
# BlueTooth
PORT = '/dev/tty.HC-05-DevB'
SPEED = 38400
a = Iduino(PORT, SPEED)
```

That's it! You're all up and running. If you're using IPython notebook, try out the examples in the attached notebook. Otherwise, try out these examples in your favorite IDE/editor:

### Simple Blink program
```python
for i in range(10):
    a.digitalWrite(13, HIGH)
    time.sleep(1)
    a.digitalWrite(13, LOW)
    time.sleep(0.3)
```

### Servo Random Sweep
```python
import numpy as np
import matplotlib.pyplot as plt
from IPython import display

f, ax = plt.subplots()
ax.set_title("Servo Control")
ax.set_xlabel('t')
ax.set_ylabel('position')
f.set_size_inches(18, 6)

myServo = Servo(a)
myServo.attach(9) # attach on pin 9

x = []
y = []
tmax = 60
t_delta = 0.5

for n in range(1,tmax):
    pos = np.random.randint(180)
    myServo.write(pos) # move to position
    
    x.append(n*t_delta)
    y.append(pos)
    
    ax.plot(x, y, 'r-')
    display.clear_output(wait=True)
    display.display(f)
    time.sleep(t_delta)

# close the figure at the end, so we don't get a duplicate
# of the last plot
plt.close()
```

### Uploading a program
```python
prog = '''

pinMode(13, OUTPUT);
for(;;) {
    digitalWrite(13, HIGH);
    delay(400);
    digitalWrite(13, LOW);
    delay(200);
}
'''
a.upload(prog)
```
