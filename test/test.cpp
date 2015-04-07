/* this file tests stedos */
#include "stedos.h"
#include <cassert>
#include <iostream>

using namespace std;

bool fired[8] = { false, false, false, false, false, false, false, false };

void timerCallback(void* data)
{
	long idx = (long) data;
	fired[idx] = true;
	cout << "timerCallback : " << (long) data << endl;
}

void test_multiple_add(void)
{
	auto timer = Timer< SimpleTimerImplementation<4> >();
	uint8_t h0 = timer.addEvent(4, timerCallback, (void*) 1);
	uint8_t h1 = timer.addEvent(1, timerCallback, (void*) 2);
	uint8_t h2 = timer.addEvent(2, timerCallback, (void*) 3);
	uint8_t h3 = timer.addEvent(2, timerCallback, (void*) 4);
	uint8_t h4 = timer.addEvent(1, timerCallback, (void*) 5);

	assert((h0 == 0x00) && "h0 != 0");	
	assert((h1 == 0x01) && "h1 != 1");
	assert((h2 == 0x02) && "h2 != 2");
	assert((h3 == 0x03) && "h3 != 3");
	assert((h4 == 0xff) && "h4 != invalid");

	/* Now test the firing of the handles */
	timer.increment();
	assert((fired[2] == true) && "fired[2]");

	timer.increment();
	assert((fired[3] == true) && "fired[3]");
	assert((fired[4] == true) && "fired[4]");

	timer.increment();
	assert((fired[1] == false) && "fired[1]");
	timer.increment();
	assert((fired[1] == true) && "fired[1]");
}

int main(void)
{
	test_multiple_add();
}