#!/usr/bin/env python

import time, sys
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

button = 21

GPIO.setup(button, GPIO.IN)

hold = 0
while True:
    # hold detection accurate to 0.25 sec
    while GPIO.input(button) == GPIO.HIGH and hold <= 12:
        hold += 1
        time.sleep(0.25)
    # if button was held, exit 1
    if hold > 12:
        sys.exit(1)
    # if button was pressed, exit 0
    if hold > 0:
        sys.exit(0)
    # debounce
    time.sleep(0.05)
