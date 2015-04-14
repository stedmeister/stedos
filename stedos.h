/*
 * StedOS
 *
 * A simple AVR 'operating system'
 *
 * (c) stedmeister
 *
 * Licesnse TBD
 */

 namespace stedos
 {
    /**********************************************************
     *
     * Atomic
     *
     **********************************************************/

    struct Atomic
    {
        Atomic()  { cli(); }
        ~Atomic() { sei(); asm volatile ("" ::: "memory"); }
    };
    /**********************************************************
     *
     * Useful storage classes
     *
     **********************************************************/

    /**
     * FIFO - Items are pushed onto the end
     *        and read from the front
     *
     * Template Parameters:
     *      T - class of the data that needs to be stored
     *   SIZE - max items (currently limited to a max size of 256)
     */
    template <typename T, int SIZE>
    class FIFO
    {
        /* check the parameters */
        static_assert(SIZE > 0, "");
        static_assert(SIZE <= 256, "");

        /* MASK is used if SIZE if a multiple of 2
           it can be used to limit the index by an &=
           operation.  */
        static const uint8_t MASK = (SIZE - 1);

    public:
        /** Adds an item to the end of the queue */
        void        push(const T& v)
        {
            auto a = Atomic();
            inc(head);
            array[head] = v;
        }

        /** Removes an item from the front of the queue */
        const T&    pop()
        {
            auto a = Atomic();
            inc(tail);
            return array[tail];
        }

        /** Looks at the next item to be popped */
        const T&    peek()
        {
            auto a = Atomic();
            uint8_t temp = tail;
            inc(temp);
            return array[temp];
        }

        /** Checks to see if the buffer is empty */
        uint8_t     isEmpty()
        {
            auto a = Atomic();
            return head == tail;
        }

        FIFO() : head(0), tail(0) {};


    private:
        T array[SIZE];    /* Allocate the memory to store the items */
        uint8_t head;     /* Keep an index to the head of the circular buffer */
        uint8_t tail;     /* Keep an index to the tail of the circular buffer */
      
        /** This function increments the indices.
            It increments the number and then limits it to SIZE.
            If SIZE is 256, then index rollover is used
            If SIZE is a multiple of 2, then MASK can be used
         */
        void inc(uint8_t& v)
        {
            //v += 1;

            //asm ( "lds %0, %1" : "=r" (v) : "" (&v));
            /* Force the compiler to put v into a local register
               generates lds r30, 0x0100 instead of ldi r26, 01, ldi r27, 00, ld r30, X */
            asm volatile("" : "=b" (v) : "0" (v));
            v += 1;

            if (SIZE == 256)
            {
                /* Nothing to do */
            }
            else if ((SIZE == 2)  || (SIZE == 4)  || (SIZE == 8)   || (SIZE == 16)   ||
                     (SIZE == 32) || (SIZE == 64) || (SIZE == 128))
            {
                v &= MASK; /* use MASK to limit index (optimisation if SIZE is a power of 2) */
            }
            else
            {
                if(v == SIZE)
                {
                    v = 0;  // use SIZE to limit index
                }
            }
        }

    };


    /* Array - handles an array of items */

    template<typename T, int SIZE>
    class Array
    {
        static_assert(SIZE > 0, "");
        static_assert(SIZE <= 256, "");

    public:
        /* Adds an item to the back of the array */
        void     append(const T& v) { array[length] = v; length += 1; }

        /* Adds an item at the specified index */
        void     insert(const T& v, uint8_t idx=0) { /* Not implemented */ }

        /* Removes an item at the specified index */
        const T& remove(uint8_t idx) { /* Not implemented */  }

        /* Removes an item at the back of the array */
        const T& pop()              { length -= 1; return array[length]; }

        const T& operator[] (uint8_t idx) { return array[idx]; }

    private:
        T array[SIZE];
        uint8_t length;
    };



    /**********************************************************
     *
     * Event processing
     *
     **********************************************************/


    /* Define a function prototype to use for structures */
    typedef void (*event_func_t) (uintptr_t data);

    /* Define a struct used to store a function pointer
       and the user data associated with it
    */

    struct Event
    {
        event_func_t func;
        uintptr_t    data;

        Event() {};
        Event(event_func_t f) : func(f) {};
        Event(event_func_t f, uintptr_t d) : func(f), data(d) {};

    };

    class EventProcessorInterface
    {
    public:
        virtual void queueEvent(event_func_t func) =0;
        virtual void queueEvent(event_func_t func, uintptr_t data) =0;
        virtual void queueEvent(const Event& event) =0;
        virtual void process() = 0;
    };

    template <int SIZE>
    class EventProcessor : public EventProcessorInterface
    {
    public:
        /** Adds an event to the queue */
        void queueEvent(event_func_t func)                 { events.push(Event(func));   }
        void queueEvent(event_func_t func, uintptr_t data) { events.push({ func, data}); }
        void queueEvent(const Event& event)                { events.push(event);         }
        void process()
        {
            while(events.isEmpty() == false)
            {
                Event event = events.pop();
                event.func(event.data);
            }
        }

    private:
        FIFO<Event, SIZE> events;
    };

    /**********************************************************
     *
     * Delayed Event processing (Timer Module)
     *
     **********************************************************/

    /* The Timer Module uses policy based class design to implement
       the actual details of the timer.  This is so that the timer code
       can be optimised for the application required.

       Timer Implementations should inherit from this base class
    */
    class TimerImplementationInterface
    {
    public:
        /* tick() is called on every timer tick */
        virtual void    tick(void) = 0;

        /* add() is called to add a new timer */
        virtual uint8_t add(uint16_t timeout, Event event) = 0;

        /* remove() is called to remove a timer, using the handle returned by add */
        virtual void    remove(uint8_t handle);
        
    };

    /* 
       Simple Timer Implmentation.  This maintains a list of
       timers, which are decremented on each tick.  When the count is 0,
       the callback is executed.
    */

    /* Struct to keep information on each timer */
    struct TimerQueue_t
    {
        uint16_t ticks;     /* Ticks remaining             */
        Event    event;     /* Event to activate on expiry */
    };

    template<int SIZE>
    class SimpleTimerImplementation : public TimerImplementationInterface
    {
    public:
        /* Constructor */
        SimpleTimerImplementation(EventProcessorInterface* p) : processor(p) {};

        /* tick() adds a timer event to the queue */
        void tick(void)
        {
            auto a = Atomic();
            /* Go through all of the queue items.
               Decrement them and call the callback if necessary.  */
            for (uint8_t idx=0; idx<SIZE; idx+=1)
            {
                if (queue[idx].ticks > 0)
                {
                    queue[idx].ticks -= 1;

                    if (queue[idx].ticks == 0)
                    {
                        processor->queueEvent(queue[idx].event);
                    }
                }
            }
        }

        /* add an event to be timed */
        uint8_t add(uint16_t timeout, Event event)
        {
            auto a = Atomic();
            for (uint8_t idx=0; idx<SIZE; idx+=1)
            {
                if (queue[idx].ticks == 0)
                {
                    queue[idx].ticks = timeout;
                    queue[idx].event = event;
                    return idx;
                }
            }

            return 0xff; /* Invalid Handle */
        }

        /* Remove handle from the list */
        void    remove(uint8_t handle)
        {
            auto a = Atomic();
            queue[handle].ticks = 0;
        }

    private:
        TimerQueue_t queue[SIZE];
        EventProcessorInterface* processor;
    };

    template<class T>
    class Timer : public T
    {
        //virtual void callback(callbackFunction f, void* userData) { f(userData); };
    };

}
/*
static __inline__ uint8_t __iCliRetVal(void)
{
    cli();
    return 1;
}

#define ATOMIC_RESTORESTATE uint8_t sreg_save \
        __attribute__((__cleanup__(__iRestore))) = SREG

#define ATOMIC_BLOCK(type) for ( type, __ToDo = __iCliRetVal(); \
                               __ToDo ; __ToDo = 0 )


for (uint8_t sreg_save = SREG, __ToDo = cli(); __ToDo; __ToDo = 0)
{

}
*/
#if 0
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


 }


#include <iostream>
using namespace std;


#include <stdint.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <iostream>
#include <cassert>

using namespace std;


class DebugIface
{
public:
    virtual void push() = 0;
    virtual void pop() = 0;
};

class NoDebug : DebugIface
{
public:
  void push() {};
  void pop() {};
};

class Debug : DebugIface
{
public:
  void push() { ++size; if (size > max_size) max_size = size; }
  void pop() { --size; }
  Debug() : size(0), max_size(0) {};

  uint16_t size;
  uint16_t max_size;

};
template <typename T, int SIZE, typename DEBUG=NoDebug>
    class FIFO
    {
      static_assert(SIZE > 0, "");
      static_assert(SIZE <= 256, "");

      const uint8_t MASK = (SIZE - 1);

      T array[SIZE];
      uint8_t head;
      uint8_t tail;
      

    public:
      void        push(const T& v) { debug.push(); inc(head);  array[head] = v; }
      const T&    pop()            { debug.pop();  inc(tail);  return array[tail]; }
      const T&    peek()           { uint8_t temp = tail; inc(temp); return array[temp]; }
      FIFO() : head(0), tail(0) {};

      DEBUG debug;

    protected:
      void inc(uint8_t& v)
      {
        v += 1;

        if((SIZE == 2)  || (SIZE == 4)  || (SIZE == 8)   || (SIZE == 16)   ||
           (SIZE == 32) || (SIZE == 64) || (SIZE == 128) || (SIZE == 256))
        {
          v &= MASK; // use MASK to limit index (optimisation if SIZE is a power of 2)
        }
        else
        {
          if(v == SIZE)
          {
            v = 0;  // use SIZE to limit index
          }
        }
      }

    };
int main(void)
{
  const int BUF_SZ = 4;
  FIFO<char, BUF_SZ, Debug> buffer;

  const char hello[] = "helloooab";
  for(int j=0; j<BUF_SZ; ++j)
  {
    char letter = 'a' + (j);
    //cout << "push : " << j << " " << letter << endl;
    buffer.push(letter);
  }

  // Read item from buffer
  for(int i=0; i<BUF_SZ; ++i)
  {
    //char v = buffer.peek();
    char w = buffer.pop();
    buffer.push(w);
    //PORTB = w;
    //cout << "pop  : " << i << " " << v << w << endl;
  }

  cout << "Debug : " << buffer.debug.max_size << endl;

  return 0;
}


-------------------------------------------------------------------------------------------


#include <stdint.h>
#include <avr/io.h>
#define NULL (0)

/* Define the list of port addresses
   This is an indexable list of addresses, that is
   filled in based on the hardware ports that
   are available on the AVR.
   If the relevant #define exist, then the table
   entry is populated with the appropriate address.
   If the relevant #define does not exist, then
   the entry is NULL.
*/
volatile uint8_t* const port_table[] = 
{
  #ifdef PORTA
    &PORTA,
  #else
    NULL,
  #endif
  #ifdef PORTB
    &PORTB,
  #else
    NULL,  
  #endif
  #ifdef PORTC
    &PORTC,
  #else
    NULL,
  #endif
  #ifdef PORTD
    &PORTD,
  #else
    NULL,
  #endif
  #ifdef PORTE
    &PORTE,
  #else
    NULL,
  #endif
  #ifdef PORTF
    &PORTF,
  #else
    NULL,
  #endif
  #ifdef PORTG
    &PORTG,
  #else
    NULL,
  #endif
};

/* Here we define the class that controls access to the
   port.  It takes a template argument as the index to
   the port address.  */
template<int PORT_ID>
struct port {
  volatile uint8_t* const address = port_table[PORT_ID];
  uint8_t operator |= (uint8_t v) { return *address |= v; }
  uint8_t operator &= (uint8_t v) { return *address &= v; }
  uint8_t read() const { return *address; }
  void write(uint8_t v) { *address = v; }
};

/* Finally we define our own class instances to access
   each port.  These will only be defined if the relevant 
   #define exists.  This is what should be used to access
   the ports.  */

#ifdef PORTA
  using port_a = port<0>;
#endif
#ifdef PORTB
  using port_b = port<1>;
#endif
#ifdef PORTC
  using port_c = port<2>;
#endif
#ifdef PORTD
  using port_d = port<3>;
#endif
#ifdef PORTE
  using port_e = port<4>;
#endif
#ifdef PORTF
  using port_f = port<5>;
#endif
#ifdef PORTG
  using port_g = port<6>;
#endif


/* This device class allows us to access a subset of the
   bits of a port.  It is arguably more useful than port as
   very rarely, can you talk to an entire port at once.


*/
/* This class template allows us to talk to a group of pins */
namespace cpp11 {
  template<bool B, class T = void>
  struct enable_if {};
 
  template<class T>
  struct enable_if<true, T> { typedef T type; };
}


template <typename PORT, int OFFSET, int LENGTH=1>
struct device
{
  static_assert(LENGTH >  0, "");
  static_assert(LENGTH <= 8, "");
  static_assert(OFFSET <  8, "");

  const uint8_t MASK = ((1 << LENGTH) - 1) << OFFSET;
  PORT port;

  template<int FOO=LENGTH>
  typename cpp11::enable_if<(FOO > 1)>::type
  set(uint8_t v) { uint8_t temp = port.read(); temp &= ~MASK; temp |= v; port.write(temp); }

  template<int FOO=LENGTH>
  typename cpp11::enable_if<(FOO == 1)>::type
  set(uint8_t v, int = 0) { if(v) set(); else clr(); }

  /*
  void set(uint8_t v) {
    if (LENGTH == 1) {
      if(v) set(); else clr();
    } else {
      uint8_t temp = port.read(); temp &= ~MASK; temp |= v; port.write(temp);
    }
  }
*/
  void set()          { port |=  MASK; }
  void clr()          { port &= ~MASK; }
  void tgl()          { port ^=  MASK; }
  uint
};

template <typename PORT, int OFFSET, int LENGTH=1>
struct read_only_device
{
  static_assert(LENGTH >  0, "");
  static_assert(LENGTH <= 8, "");
  static_assert(OFFSET <  8, "");

  const uint8_t MASK = ((1 << LENGTH) - 1) << OFFSET;
  PORT port;
};

/*
template <typename PORT, int OFFSET>
struct device<PORT, OFFSET, 1>
{
  PORT port;
  void set() { port |=  (1 << OFFSET); }
  void clr() { port &= ~(1 << OFFSET); }
  void tgl() { port ^=  (1 << OFFSET); }
};
*/
int main(void)
{
  device<port_b, 1> led;

template <typename PORT, int OFFSET, int LENGTH=1>
using read_only_device = device<PORT, OFFSET, LENGTH, read_only>;

  read_only_device<port_b, 1> led;
  read_only_device<port_b, 1> led;

  device<port_b, 2, 2> blah;
  led.set((uint8_t) 1);
  blah.set(2);

  return 0;
}


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

#endif