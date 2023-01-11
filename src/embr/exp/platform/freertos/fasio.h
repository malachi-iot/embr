#pragma once

#include <estd/functional.h>
#include <estd/port/freertos/queue.h>
#include <estd/port/freertos/thread.h>

#include "freertos/ringbuf.h"
#include "esp_log.h"

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

struct ring_buffer
{
    RingbufHandle_t h;

    void create(size_t sz, RingbufferType_t type)
    {
        h = xRingbufferCreate(sz, type);
    }

    void create(size_t sz, RingbufferType_t type,
        uint8_t* pucRingbufferStorage,
        StaticRingbuffer_t* pxStaticRingbuffer)
    {
        h = xRingbufferCreateStatic(sz, type, pucRingbufferStorage, pxStaticRingbuffer);
    }

    void free()
    {
        vRingbufferDelete(h);
    }

    BaseType_t send_acquire(void **ppvItem, size_t xItemSize, TickType_t xTicksToWait)
    {
        return xRingbufferSendAcquire(h, ppvItem, xItemSize, xTicksToWait);
    }

    BaseType_t send_complete(void *pvItem)
    {
        return xRingbufferSendComplete(h, pvItem);
    }

    void* receive(size_t* sz,  TickType_t xTicksToWait)
    {
        return xRingbufferReceive(h, sz, xTicksToWait);
    }

    void return_item(void* pvItem)
    {
        vRingbufferReturnItem(h, pvItem);
    }

    operator RingbufHandle_t() const { return h; }
};

// Depends on non-standard esp-idf ring buffer
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_additions.html#ring-buffer-api
struct fasio2
{
    ring_buffer buffer;
    int counter = 0;

    static constexpr const char* TAG = "fasio2";

    fasio2()
    {
        buffer.create(512, RINGBUF_TYPE_NOSPLIT);
    }

    struct item_base
    {
        estd::freertos::wrapper::task owner;
        uint32_t ulValue;
    };

    template <typename F>
    struct item : item_base
    {
        estd::experimental::inline_function<F, void(void)> delegate;

        item(F&& f) : delegate(std::move(f)) {}
    };

    // Think of this as a brute force union
    struct item_assist : item_base
    {
        estd::detail::function<void(void)> delegate;
    };

    template <class F>
    uint32_t begin_invoke(F&& f)
    {
        typedef item<F> item_type;

        void* pvItem;

        buffer.send_acquire(&pvItem, sizeof(item_type), portMAX_DELAY);

        ESP_LOGI(TAG, "begin_invoke: sz=%u", sizeof(item_type));

        auto i = new (pvItem) item_type(std::move(f));

        i->owner = xTaskGetCurrentTaskHandle();
        i->ulValue = ++counter;

        buffer.send_complete(pvItem);

        return i->ulValue;
    }

    void service()
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive(&sz, portMAX_DELAY);

        i->delegate();
        i->owner.notify(i->ulValue, eSetValueWithOverwrite);
        i->~item_assist();

        buffer.return_item(i);
    }


    void end_invoke(uint32_t value)
    {
        uint32_t v;

        do
        {
            xTaskNotifyWait(0, 0, &v, portMAX_DELAY);

        } while (v != value);
    }
};

}}