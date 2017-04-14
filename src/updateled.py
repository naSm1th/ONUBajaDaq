#!/usr/bin/env python

import sys
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

red = 12
green = 20
blue = 16

GPIO.setup(red, GPIO.OUT)
GPIO.setup(green, GPIO.OUT)
GPIO.setup(blue, GPIO.OUT)

if (len(sys.argv) > 1):
    if (sys.argv[1] == "red"):
        GPIO.output(red, 0)
        GPIO.output(green, 1)
        GPIO.output(blue, 1)

    elif (sys.argv[1] == "green"):
        GPIO.output(red, 1)
        GPIO.output(green, 0)
        GPIO.output(blue, 1)

    elif (sys.argv[1] == "blue"):
        GPIO.output(red, 1)
        GPIO.output(green, 1)
        GPIO.output(blue, 0)
    
    else:
        GPIO.output(red, 1)
        GPIO.output(green, 1)
        GPIO.output(blue, 1)

else:
    GPIO.output(red, 1)
    GPIO.output(green, 1)
    GPIO.output(blue, 1)
