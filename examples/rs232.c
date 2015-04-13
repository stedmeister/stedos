/* Implementation of an rs232 protocol using stedOS */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../stedos.h"

/* Create process queue */
stedos::EventProcessor<32> queue;

/* create a data pipeline */
stedos::FIFO<uint8_t, 128> pipeline;

/* Create some data structures for sending / receiving data */
stedos::FIFO<char, 32> receive_buffer;
stedos::FIFO<char, 50> transmit_buffer;

struct ev { uint8_t flag; uint8_t data; };

stedos::FIFO<ev, 8> q;


void transmit_char(const char& c)
{
	transmit_buffer.push(c);
	UCSR0B |= _BV(UDRIE0);
}

void cmd_received(uintptr_t data)
{
    /* echo the string back */
    while (transmit_buffer.isEmpty() == false)
    {
    	char c = receive_buffer.pop();
    	transmit_buffer.push(c);
    }
}

void data_received(uintptr_t data)
{
	//char letter = reinterpret_cast<intptr_t>(data);
	//char letter = (char) data;
	//receive_buffer.push(letter);

	//if (letter == '\n')
	//	queue.queueEvent(cmd_received, 0);
}

void queue_empty(uintptr_t data)
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
	//stedos::data_t c = UDR0;
	//char c = UDR0;
	//receive_buffer.push(UDR0);
	//queue.queueEvent(data_received);

	queue.queueEvent(data_received, UDR0);
	//q.push({3, UDR0});
	//pipeline.push(0x34);
	//pipeline.push(c);
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

/*
struct PtrEvent : public Event
{
	PtrEvent(stedos::event_func_t f, void* ptr)
	{
		func = f;
		p = ptr;
	}

	void fire()
	{
		//stedos::event_func_t f = (stedos::event_func_t) func;
		func(p);
	}
};

struct CharEvent : public Event
{
	CharEvent(stedos::event_func_t f, uint8_t c)
	{
		func = f;
		u8 = c;
	}

	void fire()
	{
		func(u8);
	}
};
*/

int main(void)
{
	/*
	uint8_t c = 8;
	PtrEvent p(cmd_received, &c);
	p.fire();

	CharEvent q(cmd_received, c);

	ReceiveChar rec('a');
	rec.fire();
*/

	auto timer = stedos::SimpleTimerImplementation<12>(&queue);
	timer.add(100, {data_received});

	setup_rs232();
	sei();

	foo(PORTC);	

	while(1)
		queue.process();

}
