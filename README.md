# stedos
Simple AVR operating system.

Introduction
============

In my first job after university, I was an AVR programmer.  I really love AVRs for the following reasons:

* Clear and well written datasheets
* Can program using C and GCC
* Easy to use hardware blocks

After a while, I came up with the following construct for creating programs.  For each logic software block e.g. rs232, adc, some other processing block, I would create an `init()` and a `loop()` function.  Then my main code block would look something like the following:

```
int main(void)
{
	adc_init();
	rs232_init();
	process_adc_init();

	while(1)
	{
		adc_loop();
		rs232_loop();
		process_adc_loop();
	}
}
```

Using this method, it was possible to give each block a slice of the processor time.  It was still necessary for each `loop()` method to not take so long so that one block didn't become starved of processor time.

I always wanted to come up with something slightly better.  I did consider using a hardware timer to trigger an interrupt that would interrupt the current thread context and restore another one.  I was put off for two reasons:

* The number of instructions required to switch contexts seemed high.
* It would be very difficult to debug.

Event Based Processing
======================

I was inspired by Javascript and considered turning to an event based system.  Instead of storing state in a variable that would determine what happened during the `loop()` function, we setup a number of callback functions to be called in turn.  This is easier to demonstrate with some code:

```
/* Define our callback function type */
typedef void (*callbackFunction) (void* data);

/* Create a struct that stores the function pointer
   and the user data to be passed.
 */
typedef struct
{
	callbackFunction function;
	void* data;
} process_t;

void initADC(void* data)
{
	/* Function gets filled in here */
}

int main (void)
{
    /* Initialise queue */
	process_t array[NUM_ITEMS];

    /* Add some events to queue.  They are not called yet */
	addEventToQueue(initADC, NULL);

    /* Start processing loop */
	while(1)
	{
		/* Try and read an event from the queue */
		process_t* next_event = getEventFromQueue();
		if (next_event)
		{
			/* If there is one, call it */
			next_event->function(next_event->data);
		}
	}
}
```

I really like this because it can be used to quickly handle interrupt routines and it provides a way of pretending to be multiprocess.  Consider the following example which receives some data over RS232.  Assume that each data packet consists of 3 header bytes and 5 data bytes.  This gives rise to the follwoing code:

```
/* Interrupt service routine for handling arrival of data */
ISR(USART_Rx_Data)
{
	/* read byte from hardware register and put it onto the queue */
	addEventToQueue(byteReceived, (void*) byte);
}

void byteReceived(void* byte)
{
	/* Add byte to buffer */
	buffer[bufferIdx++] = (uint8_t) byte;

    /* Check to see if we have received the entire packet */
    if(bufferIdx == 8)
    {
    	addEventToQueue(packetReceived, &buffer[3]);  /* Return address to start of data */
    }
}

void packetReceived(void* data)
{
	/* Do something with the data */
}
```

This seems really clean to me.  The arrival of the interrupt triggers a number of events that get handled in turn.  The other thing that is really cool is that a series of events will only be triggered from an interrupt routine.  This means that the main process loop can put the AVR to sleep if there is nothing to process, as it will be woken when there is something to do.