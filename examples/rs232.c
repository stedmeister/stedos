/* Implementation of an rs232 protocol using stedOS */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../stedos.h"

/* Create some data structures for sending / receiving data */
stedos::FIFO<char, 32> receive_buffer;
stedos::FIFO<char, 32> transmit_buffer;

/* Create process queue */
stedos::EventProcessor<32> queue;

void transmit_char(const char& c)
{
	transmit_buffer.push(c);
	UCSR0B |= _BV(UDRIE0);
}

void cmd_received(void* data)
{
    /* echo the string back */
    while (transmit_buffer.isEmpty() == false)
    {
    	char c = receive_buffer.pop();
    	transmit_buffer.push(c);
    }
}

void data_received(void* data)
{
	char letter = reinterpret_cast<intptr_t>(data);
	//char letter = (char) data;
	receive_buffer.push(letter);

	if (letter == '\n')
		queue.queueEvent(cmd_received, 0);
}

void queue_empty(void* data)
{
	if (transmit_buffer.isEmpty() == false)
	{
		UDR0 = transmit_buffer.pop();
		UCSR0B |= _BV(UDRIE0);
	}
}

/* Hook into the interrupts */
ISR(USART_RX_vect)
{
	queue.queueEvent(data_received, (void*) UDR0);
}

ISR(USART_UDRE_vect)
{
	queue.queueEvent(queue_empty, 0);
	UCSR0B &= ~_BV(UDRIE0);
}

void setup_rs232()
{
	const uint8_t ubrr = 103;

	UBRR0H = 0;
	UBRR0L = ubrr;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void foo(uint16_t i)
{
	uint8_t a = i;
	PORTB = a;
}


int main(void)
{
	setup_rs232();
	sei();

	foo(PORTC);	

	while(1)
		queue.process();

}
