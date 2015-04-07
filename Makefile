all: a.out

a.out: stedos.h test.cpp
	g++ test.cpp -std=c++11
	#avr-g++ test.cpp -ffunction-sections -fdata-sections -Wl,--gc-sections

clean:
	rm a.out
