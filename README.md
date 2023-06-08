# RepkaPi.GPIO

## Installation

    sudo apt-get update
    sudo apt-get install python-dev git
    git clone https://github.com/DikShv/RepkaPi.GPIO.git
    cd RepkaPi.GPIO
    sudo python setup.py install

## Supported Boards

* Repka Pi 3

## Usage

Same as RPi.GPIO but with a new function to choose Repka Pi Board.

    import RepkaPi.GPIO as GPIO
    GPIO.setboard(GPIO.REPKAPI3)
    GPIO.setmode(GPIO.BOARD)
    GPIO.output(5, 1)

Many demo is on the example folder
