#!/usr/bin/env python
# -*- coding: utf-8 -*-


from RepkaPi import GPIO

from time import sleep          # позволяет выставить задержку на время

GPIO.setboard(GPIO.REPKAPI3)        # Repka Pi 3
GPIO.setmode(GPIO.BOARD)        # выбираем тип обращения к GPIO по номеру PIN (BOARD)
GPIO.setup(15, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)    # устанавливаем pin 15 в input (Кнопка)
GPIO.setup(11, GPIO.OUT)        # устанавливаем pin 11 как output (LED)

try:
    while True:                 # цикл будет выполняться пока не нажмем CTRL+C
        if GPIO.input(15) == 1:      # if pin 15 == 1
            print ("PIN 15 равен 1/HIGH/True - LED ON")
            GPIO.output(11, 1)  # выставляем pin 11 1/HIGH/True
        else:
            print ("PIN 15 равен 0/LOW/False - LED OFF")
            GPIO.output(11, 0)  # выставляем pin 11 0/LOW/False
        sleep(0.1)              # задержка 0.1 секунда

finally:                        # выполняет блок инструкций в любом случае, было ли исключение, или нет
    print("Finally")
    GPIO.output(11, 0)
    GPIO.cleanup()              # убираем все настойки по GPIO
