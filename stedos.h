/* This class is a funky class */
#include <iostream>
using namespace std;

/* Array */
template <class T, int maxItems>
class Array
{
public:
	Array() : currentSize(0) {};
	T& operator[](int idx) { return this->items[idx]; };
	void pushBack(T& item) { items[currentSize++] = item; };

private:
	uint16_t currentSize;
	T items[maxItems];
};

template <class T, int maxItems>
class LinkedList
{
public:
	LinkedList() : first(NULL), last(NULL) {};
	void pushBack(T& item)
	{
		ItemData* item = addItem(item);
		item->next = NULL;
		this->last->next = item;
		this->last = item;

		if(this->first == NULL) this->first = item;
	};
	void pushFront(T& item)
	{
		ItemData* item = addItem(item);
		item->next = this->first;
		this->first = item;

		if(this->last == NULL) this->last = item;
	};
private:
	class ItemData
	{
		uint8_t inUse;
		T data;
		ItemData* next;
	} items[maxItems];

	ItemData* first;
	ItemData* last;

	ItemData* addItem(T& item)
	{
		for(uint16_t idx=0; idx<maxItems; idx+=1)
		{
			if (items[idx].inUse == 0)
			{
				items[idx].data = item;
				items[idx].inUse = 1;
				return &items[idx];
			}
		}
		return NULL;
	}
};

/* Callback definitions */
typedef void (*callbackFunction) (void* data);

class ProcessInterface
{
public:
	virtual void queueEvent(callbackFunction f, void* data);
};

typedef struct
{
	callbackFunction callback;
	void* data;
} QueueData_t;

template <int maxEvents>
class SimpleProcess : public ProcessInterface
{
public:
	virtual void queueEvent(callbackFunction f, void* data);

private:
	QueueData_t eventQueue[maxEvents];
};

template <class T>
class Process : public T
{
};

template <int maxEvents>
void SimpleProcess<maxEvents>::queueEvent(callbackFunction f, void* data)
{

}




class TimerInterface
{
public:
	virtual void    increment(void) = 0;
	virtual uint8_t addEvent(uint16_t timeout, callbackFunction f, void* userData) = 0;
	virtual void    callback(callbackFunction f, void* userData) = 0;
};

/* This class maintains a simple list.
   It decrements the timer for each time.
*/
typedef struct
{
	uint16_t ticks;
	callbackFunction callback;
	void* userData;
} TimerQueue_t;

template<int queueLength>
class SimpleTimerImplementation : public TimerInterface
{
public:
	virtual void increment(void);
	virtual uint8_t addEvent(uint16_t timeout, callbackFunction f, void* u);

private:
	TimerQueue_t queue[queueLength];
};

template<class T>
class Timer : public T
{
	virtual void callback(callbackFunction f, void* userData) { f(userData); };
};

template<int queueLength>
void SimpleTimerImplementation<queueLength>::increment(void)
{
	//cout << "SimpleTimerImplementation::increment" << endl;
	/* Go through all of the queue items.
	   Decrement them and call the callback if necessary.  */
	for (uint8_t idx=0; idx<queueLength; idx+=1)
	{
		if (this->queue[idx].ticks > 0)
		{
			this->queue[idx].ticks -= 1;

			if (this->queue[idx].ticks == 0)
			{
				this->callback(this->queue[idx].callback, this->queue[idx].userData);
				//cout << "callback " << idx << endl;
			}
		}
	}
}

template<int queueLength>
uint8_t SimpleTimerImplementation<queueLength>::addEvent(uint16_t timeout, callbackFunction f, void* userData)
{
	//cout << "SimpleTimerImplementation::addEvent" << endl;

	/* Go through the queue list, find a spare space for this item */
	for (uint8_t idx=0; idx<queueLength; idx+=1)
	{
		if (this->queue[idx].ticks == 0)
		{
			this->queue[idx].ticks = timeout;
			this->queue[idx].callback = f;
			this->queue[idx].userData = userData;
			return idx;
		}
	}

	return 0xff; /* Invalid Handle */
}
