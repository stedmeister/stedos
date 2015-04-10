/* this file tests stedos */
#include <stdint.h>
#include "../stedos.h"
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
	cout << "test_multiple_add" << endl;
	#if 0
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
	#endif
}

/* This test overflows the buffers
   and then tests that tne buffers have
   been overwritten correctly
 */
void test_FIFO(void)
{
	cout << "test_FIFO" << endl;
	stedos::FIFO<char, 8> fifo8;
	stedos::FIFO<char, 5> fifo5;

    for (int i=0; i<11; ++i)
    {
    	fifo8.push('a' + i);
		fifo5.push('a' + i);
    }

    /* Read the characters back out */
    assert((fifo5.peek() == 'k') && "peek fifo5[0]");
	assert((fifo5.pop()  == 'k') && "pop  fifo5[0]");
	assert((fifo5.peek() == 'g') && "peek fifo5[1]");
	assert((fifo5.pop()  == 'g') && "pop  fifo5[1]");
	assert((fifo5.peek() == 'h') && "peek fifo5[2]");
	assert((fifo5.pop()  == 'h') && "pop  fifo5[2]");
	assert((fifo5.peek() == 'i') && "peek fifo5[3]");
	assert((fifo5.pop()  == 'i') && "pop  fifo5[3]");
	assert((fifo5.peek() == 'j') && "peek fifo5[4]");
	assert((fifo5.pop()  == 'j') && "pop  fifo5[4]");

    assert((fifo8.peek() == 'i') && "peek fifo8[0]");
	assert((fifo8.pop()  == 'i') && "pop  fifo8[0]");
	assert((fifo8.peek() == 'j') && "peek fifo8[1]");
	assert((fifo8.pop()  == 'j') && "pop  fifo8[1]");
	assert((fifo8.peek() == 'k') && "peek fifo8[2]");
	assert((fifo8.pop()  == 'k') && "pop  fifo8[2]");
	assert((fifo8.peek() == 'd') && "peek fifo8[3]");
	assert((fifo8.pop()  == 'd') && "pop  fifo8[3]");
	assert((fifo8.peek() == 'e') && "peek fifo8[4]");
	assert((fifo8.pop()  == 'e') && "pop  fifo8[4]");
	assert((fifo8.peek() == 'f') && "peek fifo8[5]");
	assert((fifo8.pop()  == 'f') && "pop  fifo8[5]");
	assert((fifo8.peek() == 'g') && "peek fifo8[6]");
	assert((fifo8.pop()  == 'g') && "pop  fifo8[6]");
	assert((fifo8.peek() == 'h') && "peek fifo8[7]");
	assert((fifo8.pop()  == 'h') && "pop  fifo8[7]");

}

int main(void)
{
	test_multiple_add();
	test_FIFO();
}