#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import RepkaPi.GPIO as GPIO

GPIO.setmode(GPIO.SOC)

# Получаем модель выбранной платы
print (GPIO.getboardmodel())

print (GPIO.getmode())

# Версия библиотеки
print (GPIO.VERSION)

# Информация об устройстве
print (GPIO.RPI_INFO)

func = GPIO.gpio_function(3)
print(func)

func = GPIO.gpio_function(7)
print(func)




func_name = GPIO.gpio_function_name(3)
print(func_name)

func_name = GPIO.gpio_function_name(7)
print(func_name)


