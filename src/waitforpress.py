#!/usr/bin/env python

import time
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

button = 4

GPIO.setup(button, GPIO.IN, GPIO.PUD_UP)

while True:
    button_state = GPIO.input(button)
    time.sleep(3)
    return
    """
    if button_state == GPIO.HIGH:
        print ("H")
    else:
        print("L")
    """
    time.sleep(0.5)
