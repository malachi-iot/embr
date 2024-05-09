#include <Arduino.h>

#include <estd/ostream.h>
#include <embr/exp/thunk.h>

static estd::arduino_ostream cout(Serial);

struct Mutex
{
    static inline void lock_push() { cli(); }
    static inline void unlock_push() { sei(); }
    static inline void lock_pop() { cli(); }
    static inline void unlock_pop() { sei(); }
};

embr::experimental::layer1::Thunk<64, Mutex> thunk;
int counter = 0;

#if __AVR__
// With help from
// 1. https://exploreembedded.com/wiki/AVR_Timer_Interrupts
// 2. https://home.csulb.edu/~hill/ee346/Lectures/12%20ATmega32U4%20Timer%20Interrupts.pdf
// 3. https://ww1.microchip.com/downloads/en/Appnotes/Atmel-2505-Setup-and-Use-of-AVR-Timers_ApplicationNote_AVR130.pdf

#include <avr/io.h>
#include <avr/interrupt.h>

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
	TCNT1 = 63974;   // for 1 sec at 16 MHz

    ++counter;
    int c = counter;

    // DEBT: We're getting ~10 per second, not 1
    if(counter % 3 == 0)
    {
        // Be careful, capture of a global variable [counter] appears to be a noop
        thunk.enqueue([c]
        {
            cout << F("ISR: ") << c << estd::endl;
        });
    }
}
#endif

void setup()
{
    Serial.begin(115200);

    // DEBT: Guide indicates this is a 1s counter, but it's 100ms really
#if __AVR__
    cli();
    TCCR1A = 0x00;
    // Pck/1024 [2] p. 8
    TCCR1B = (1<<CS10) | (1<<CS12);     // Timer mode with 1024 prescler
    TIMSK1 = (1 << TOIE1);              // Enable timer1 overflow interrupt(TOIE1)
    sei();                              // Enable global interrupts by setting global interrupt enable bit in SREG
#endif
}

void loop()
{
    // AVR 'used' helps us deduce 6 bytes per entry:
    // - 16-bit size
    // - function pointer
    // - capture of counter
    unsigned used = thunk.used();
    thunk.invoke_all();
    cout << F("Counter: ");
    // DEBT: Bringing in integer printer adds nearly 500 bytes of ROM, while Arduino's is only ~250 bytes
    // Look into this in estd
    cout << counter;
    cout << ", Used: " << used;
    cout << estd::endl;
    delay(1000);
}

