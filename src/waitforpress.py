#!/usr/bin/env python

import time
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

button = 4

GPIO.setup(button, GPIO.IN, GPIO.PUD_UP)

while True:
    button_state = GPIO.input(button)
    print ("waiting for button press...")
    if button_state == GPIO.HIGH:
        print ("button is high")
    else:
        print("button is low")
    time.sleep(0.5)
    # take this out ########
    time.sleep(3)
    break 
    ########################
