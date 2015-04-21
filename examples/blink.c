/*****************************************************
 *
 * LED blink - the hello world of the microprocessor world
 *
 * This example will blink an LED at 1Hz
 *
 *****************************************************/

/* Make sure that the oscillator frequency of the
   uProcessor has been defined.  This is used by
   a few of the AVR libc files */
#define F_CPU (16000000)


/* Include the necessary AVR header files */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


/* Lastly include the stedos header file.
   This should be AFTER the svr header files as
   it relies on some defines setup by the AVR
   header files.  */
#include "../stedos.h"

/* Create a variable to represent the LED
   we are using the same pin that the
   arduino uses for its LED for simplicity */
stedos::IO<stedos::port_b, 0> led;

/* Create a process queue to handle events,
   because we only have a few events to process,
   we limit the queue depth to 4 events. */
stedos::EventProcessor<4> queue;

/* Create a timer queue for the delayed event */
auto timer = stedos::SimpleTimerImplementation<1>(&queue);


/* Create an event function that is used to
   toggle the LED.  Even though data is not
   used, the prototype must match */
void toggle_led(uintptr_t data)
{
	led.toggle();                     // toggle the LED
    timer.add(500, {toggle_led, 0});  // readd the wait
}


/* Create the main function.  This is the entry point
   of our program.  */
int main (void)
{
	/* Setup timer0 to interrupt every 500ms */
	TCCR0A = 0;             // CTC mode no pin outputs
	TCCR0B = 0x03;  // CLKDIV / 64
	TCNT0 = 250;                      // Interrupt every 1ms
	TIMSK0 |= _BV(TOIE0);             // Enable overflow interrupt

	DDRB = 0xff;

	sei();

	/* Add the timer callback to expire every 500 ms */
	timer.add(500, {toggle_led, 0});

	/* finally start the process queue */
	while(1)
	{
		queue.process();
	}

	return 0;

}

/* Finally plumb in the interrupt */
ISR(TIMER0_OVF_vect)
{
	/* Call the timer's tick function */
	timer.tick();

}

