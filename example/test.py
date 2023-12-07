#!/usr/bin/env python
# -*- coding: utf-8 -*-


from RepkaPi import GPIO

from time import sleep          # позволяет выставить задержку на время

GPIO.setboard(GPIO.REPKAPI3)        # Repka Pi 3
GPIO.setmode(GPIO.BOARD)        # выбираем тип обращения к GPIO по номеру PIN (BOARD)
pin = [3, 5, 7, 11, 13, 15, 19, 21, 23, 27, 29, 31, 33, 35, 37, 12, 16, 18, 22, 24, 26, 28, 32, 36,38, 40]
#pin = [7, 33, 16, 18, 32]
for p in pin:
    GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_UP)

sleep(2)


for p in pin:
    GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

sleep(2)

for p in pin:
    GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_UP)    # устанавливаем pin 15 в input (Кнопка)

sleep(2)


for p in pin:
    GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_OFF)    # устанавливаем pin 15 в input (Кнопка)

for p in pin:
	GPIO.setup(p, GPIO.OUT)
	GPIO.output(p, 1)
sleep(2)

for p in pin:
	GPIO.setup(p, GPIO.OUT)
	GPIO.output(p, 0)