#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import RepkaPi.GPIO as GPIO

# Получаем модель выбранной платы
print (GPIO.getboardmodel())

print (GPIO.getmode())

# Версия библиотеки
print (GPIO.VERSION)

# Информация об устройстве
print (GPIO.RPI_INFO)

