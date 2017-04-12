#!/usr/bin/env python

import time, sys
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

button = 4

GPIO.setup(button, GPIO.IN, GPIO.PUD_UP)

# print ("waiting for button press...")
raw_input("waiting for button press...\n")
sys.exit(0)

while True:
    button_state = GPIO.input(button)
    if button_state == GPIO.HIGH:
        print ("high")
    else:
        print("low")
    time.sleep(0.5)
    # remove when button works
    break;
