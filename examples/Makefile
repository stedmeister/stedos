all : rs232.lst

rs232: rs232.c
	avr-g++ -g -std=c++11 -Os -mcall-prologues -mmcu=atmega328p -fpermissive rs232.c -o rs232
	

rs232.lst: rs232
	avr-objdump rs232 -dhS > rs232.lst

clean:
	rm -f rs232.lst
	rm -f rs232