#!/usr/bin/env python
# -*- coding: utf-8 -*-

import RepkaPi.GPIO as GPIO
from time import sleep          # this lets us have a time delay

GPIO.setboard(GPIO.REPKAPI3)    # Repka Pi 3
GPIO.setmode(GPIO.SOC)        # set up BOARD BCM numbering
GPIO.setup([7, 64], GPIO.OUT)         # set pin 7 as an output (LED)
#GPIO.setup(33, GPIO.OUT)

try:
    print ("Press CTRL+C to exit")
    while True:
        GPIO.output([7, 64], 1)       # set port/pin value to 1/HIGH/True
        #GPIO.output(24, 1) 
        sleep(0.1)
        #GPIO.output(7, 0)       # set port/pin value to 0/LOW/False
        GPIO.output([7, 64], 0)
        #sleep(0.1)

       

        sleep(0.5)

except KeyboardInterrupt:
    GPIO.output(7, 0)           # set port/pin value to 0/LOW/False
    GPIO.cleanup()              # Clean GPIO
    print ("Bye.")
