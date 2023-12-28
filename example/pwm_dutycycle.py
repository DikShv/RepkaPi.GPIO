#!/usr/bin/env python
# -*- coding: utf-8 -*-

import RepkaPi.GPIO as GPIO
import time


try:
    # === Инициализация пинов ===
	GPIO.setboard(GPIO.REPKAPI3)
	GPIO.setmode(GPIO.BOARD)    # set up BOARD BCM numbering
	pinPWM=33
	GPIO.setup(pinPWM, GPIO.OUT)    # Устанавливаем pinPWM в режим OUTPUT

	pwm = GPIO.PWM(pinPWM, 100)     # создаем ШИМ-объект для пина pinPWM с частотой 100 Гц
	pwm.start(1)                   # Запускаем ШИМ на пине со скважностью 10% (0-100%)
	                                # Можно использовать данные типа float - 49.5, 2.45
	
	while 1:
		for i in range(0,101):
			pwm.ChangeDutyCycle(i)  # Изменяем коэффициент запонения (скважность) от 0 до 100%
			time.sleep(0.5)         # Пауза на 0,1 сек
		time.sleep(5)               # Пауза на 5 сек, для осциллографа
		for i in range(100,-1,-1):
			pwm.ChangeDutyCycle(i)  # Изменяем коэффициент запонения (скважность) от 100 до 0%
			time.sleep(0.2)         # Пауза на 0,1 сек
		time.sleep(5)               # Пауза на 5 сек, для осциллографа
 
except KeyboardInterrupt:
	# ...
	print("Exit pressed Ctrl+C")        # Выход из программы по нажатию Ctrl+C
except:
	# ...
	print("Other Exception")            # Прочие исключения
	print("--- Start Exception Data:")
	traceback.print_exc(limit=2, file=sys.stdout) # Подробности исключения через traceback
	print("--- End Exception Data:")
finally:
	pwm.stop()                          # Останавливаем ШИМ - необязательно

	print("CleanUp")                    # Информируем сбросе пинов
	GPIO.cleanup()                      # Возвращаем пины в исходное состояние
	print("End of program")             # Информируем о завершении работы программы
