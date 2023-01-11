#pragma once

#include <estd/functional.h>
#include <estd/port/freertos/queue.h>
#include <estd/port/freertos/thread.h>

// EXPERIMENTAL
// "fake ASIO"
// everything happens on stack and you do have to wait for completion, but you
// can do it kinda async/await style so you're not TOTALLY hung up

namespace embr { namespace experimental {

struct fasio
{
    // NOTE: In theory we could manage a psuedo dynamic memory with
    // a circular objstack here, as long as we always limit 'service'
    // to be called by just one thread/task

    struct item
    {
        estd::freertos::wrapper::task owner;
        estd::detail::function<void(void)> delegate;
        uint32_t ulValue;
    };

    uint32_t counter = 0;

    estd::freertos::layer1::queue<item, 10> queue;

    uint32_t begin_invoke(estd::detail::function<void(void)> f)
    {
        item i{xTaskGetCurrentTaskHandle(), f, ++counter};
        queue.send(i);
        return i.ulValue;
    }


    void end_invoke(uint32_t value)
    {
        uint32_t v;

        do
        {
            xTaskNotifyWait(0, 0, &v, portMAX_DELAY);
            
        } while (v != value);
    }


    // NOTE: Watch out!  Ending prematurely with stack/inline function in
    // play is a crash waiting to happen
    BaseType_t end_invoke(uint32_t value, TickType_t ticks_to_wait)
    {
        uint32_t v;

        TickType_t last = xTaskGetTickCount();
        
        do
        {
            BaseType_t r = xTaskNotifyWait(0, 0, &v, ticks_to_wait);

            // timed out
            if(r == pdFALSE) return pdFALSE;

            const TickType_t current = xTaskGetTickCount();
            const TickType_t elapsed = current - last;

            ticks_to_wait -= elapsed;
            
            last = current;

        } while (v != value);

        return pdTRUE;
    }


    // DEBT: copy/paste from estd::experimental::function really - should we just use that?
    template <typename F>
    ESTD_CPP_CONSTEXPR_FUNCTION static estd::experimental::inline_function<F, void(void)> make_inline(F&& f)
    {
        return estd::experimental::inline_function<F, void(void)>(std::move(f));
    }

    template <typename F>
    struct fat_handle
    {
        estd::experimental::inline_function<F, void(void)> storage;
    };

    template <typename F>
    static estd::experimental::inline_function<F, void(void)>
        begin_invoke2(F&& f)
    {

    }


    void service()
    {
        item i;
        
        queue.receive(&i);

        i.delegate();
        i.owner.notify_give();
    }
};

}}