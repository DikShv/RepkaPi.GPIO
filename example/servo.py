#!/usr/bin/env python
import RepkaPi.GPIO as GPIO
from time import sleep

GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)
def setServoAngle(servo, frequency_Hz, angle):
	pwm = GPIO.PWM(servo, frequency_Hz)
	pwm.start(8)
	dutyCycle = angle / 18. + 3.
	print("Угол поворота: " + str(angle) + "\xb0")
	print("Коэффициент заполнения Duty: " + str(dutyCycle) + "%")
	period_duration = 1 / frequency_Hz * 1000
	print("Длительность периода: ", period_duration, "мс, частота " + str(frequency_Hz) + " Гц")
	pulse_duration = (period_duration / 100) * dutyCycle
	print("Длительность импульса ШИМ :", round(pulse_duration, 1), "мс")
	pwm.ChangeDutyCycle(dutyCycle)
	sleep(1)
	pwm.stop()
if __name__ == '__main__':
	import sys
	servo = 11
	GPIO.setup(servo, GPIO.OUT)
	setServoAngle(servo, 50, 0)
	sleep(2)
	setServoAngle(servo, 50, 90)
	sleep(2)
	setServoAngle(servo, 50, 180)
	sleep(2)
	GPIO.cleanup()