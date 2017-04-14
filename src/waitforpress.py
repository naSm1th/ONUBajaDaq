#!/usr/bin/env python

import time
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

button = 21

GPIO.setup(button, GPIO.IN)

print ("waiting for button press...")
while True:
    button_state = GPIO.input(button)
    if button_state == GPIO.HIGH:
        print ("high")
        break;
    else:
        time.sleep(0.5)
