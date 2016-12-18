# RaspberryPi Character Driver Example
This driver controls leds and a buzzer with GPIO.
Driver has IOCTL callback to get parameters.
* +a = turn on all
* -a = turn off all
* +153 = turn on led[1], led[5] and led[3]
* -3412 = turn off led[3], led[1], led[4] and led[2]
* +b = start blink.
* -b = stop blink.
* +s = start seq. (one by one)
* -s = stop seq.
