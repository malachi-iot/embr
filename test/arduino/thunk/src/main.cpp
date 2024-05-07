#include <Arduino.h>

#include <embr/exp/thunk.h>

struct Mutex
{
    static inline void lock_push() { cli(); }
    static inline void unlock_push() { sei(); }
    static inline void lock_pop() { cli(); }
    static inline void unlock_pop() { sei(); }
};

embr::experimental::layer1::Thunk<128, Mutex> thunk;

void setup()
{
}

void loop()
{
    thunk.invoke();
}

