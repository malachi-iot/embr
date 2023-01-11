#pragma once

#include <estd/functional.h>
#include <estd/variant.h>

#include <estd/port/freertos/queue.h>
#include <estd/port/freertos/thread.h>

#include <embr/internal/argtype.h>

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


// TODO: Consider an implementation of this using xMessageBuffer for better
// cross platform ability.  Note that that is not zero copy.  ring buffer
// promises zero copy, but F&& f behaviors might interrupt that - it depends
// on if compiler truly inlines 'enqueue'
template <class TUser = estd::monostate>
struct delegate_queue
{
    static constexpr const char* TAG = "delegate_queue";

    ring_buffer buffer;

    typedef TUser item_base;

    template <typename F>
    struct item : item_base
    {
        estd::experimental::inline_function<F, void(void)> delegate;

        template <class ...TArgs>
        item(F&& f, TArgs&&...args) :
            item_base(std::forward<TArgs>(args)...),
            delegate(std::move(f))
        {}
    };

    // Think of this as a brute force union
    struct item_assist : item_base
    {
        estd::detail::function<void(void)> delegate;
    };

    // 'F' signature *must* be void(), TArgs are to construct
    // the tracking item itself
    template <class F, class ...TArgs>
    inline void enqueue(F&& f, TArgs&&...args)
    {
        typedef item<F> item_type;

        void* pvItem;

        buffer.send_acquire(&pvItem, sizeof(item_type), portMAX_DELAY);

        ESP_LOGI(TAG, "enqueue: sz=%u", sizeof(item_type));

        new (pvItem) item_type(std::move(f), std::forward<TArgs>(args)...);

        buffer.send_complete(pvItem);
    }

    template <class F>
    void dequeue(F&& f)
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive(&sz, portMAX_DELAY);

        i->delegate();

        f(i);

        i->~item_assist();

        buffer.return_item(i);
    }

    void dequeue() { dequeue([](auto){}); }

    template <class T>
    struct wrapper_promise
    {
        T retval;
    };

    template <class T>
    struct async_wrapper
    {
        T retval;

    };

    // https://fekir.info/post/how-to-force-return-value-optimization/
    // because f2 below's this->retval MUST NOT move around
    template <class T>
    class async_wrapper2
    {
        T retval_;
        bool valid_ = false;

        async_wrapper2() = delete;
        async_wrapper2(const async_wrapper2&) = delete;
        async_wrapper2(async_wrapper2&&) = delete;
        async_wrapper2 operator=(const async_wrapper2&) = delete;

    public:
        bool valid() const { return valid_; }
        T get() const { return retval_; }
        
        template <class F, class ...TArgs>
        async_wrapper2(delegate_queue q, F&& f, TArgs&&...args)
        {
            static const char* TAG = "async_wrapper2";

            //ESP_LOGI(TAG, "ctor");

            auto f2 = [&]()
            {
                retval_ = f(std::forward<TArgs>(args)...);
                valid_ = true;
            };
            q.enqueue(std::move(f2));
        }
    };

    template <class F, class ...TArgs>
    async_wrapper<estd::invoke_result_t<estd::decay_t<F>, TArgs... > > test2(F&& f, TArgs&&...args)
    {
        typedef estd::invoke_result_t<estd::decay_t<F>, TArgs... > result_type;
        async_wrapper<result_type> wrapper;
        auto f2 = [&]()
        {
            return f(std::forward<TArgs>(args)...);
        };

        wrapper.retval = f2();

        return wrapper;
    }

    template <class F, class ...TArgs>
    async_wrapper2<estd::invoke_result_t<estd::decay_t<F>, TArgs... > > test3(F&& f, TArgs&&...args)
    {
        typedef estd::invoke_result_t<estd::decay_t<F>, TArgs... > result_type;
        typedef async_wrapper2<result_type> wrapper;
        return wrapper(*this, std::move(f), std::forward<TArgs>(args)...);
    }

    template <class F>
    typename embr::internal::ArgType<F>::result_type test(F&& f)
    {
        typedef typename embr::internal::ArgType<F>::result_type result_type;

        return result_type();
    }
};


// Depends on non-standard esp-idf ring buffer
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_additions.html#ring-buffer-api
struct fasio2
{
    struct item_base
    {
        estd::freertos::wrapper::task owner;
        uint32_t ulValue;
    };

    delegate_queue<item_base> buffer;
    int counter = 0;

    fasio2()
    {
        buffer.buffer.create(512, RINGBUF_TYPE_NOSPLIT);
    }

    template <class F>
    uint32_t begin_invoke(F&& f)
    {
        buffer.enqueue(std::move(f), xTaskGetCurrentTaskHandle(), ++counter);

        return counter;
    }

    void service()
    {
        buffer.dequeue([](auto i)
        {
            i->owner.notify(i->ulValue, eSetValueWithOverwrite);
        });
    }


    void end_invoke(uint32_t value)
    {
        uint32_t v;

        do
        {
            xTaskNotifyWait(0, 0, &v, portMAX_DELAY);

        // The way task notification works we might miss one,
        // so keep checking until we get a v >= what we're looking for.
        // this is safe because delegate queue convention is to sequentially
        // process the items
        } while (v < value);
    }
};

}}