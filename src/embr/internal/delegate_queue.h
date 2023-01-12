// NOTE: Depends on esp-idf framework!  Also depends on c++11
// delegate_queue is entirely based on esp-idf's ring buffer.
// Since we might implement our own version of this ring buffer someday, putting
// delegate queue out in non-platform specific area, for the time being

#include <estd/functional.h>
#include <estd/port/freertos/ring_buffer.h>

#include "esp_log.h"

namespace embr { namespace internal {

namespace impl {

template <class TItem = estd::monostate>
struct reference_delegate_queue
{
    typedef TItem item;

    // DEBT: Imperfect, but at least there's a way to do this
    // without dancing the variadic template dance
    static void on_enqueue(TItem*) {}
    static void on_dequeue(TItem*) {}
};

}

// TODO: Consider an implementation of this using xMessageBuffer for better
// cross platform ability.  Note that that is not zero copy.  ring buffer
// promises zero copy, but F&& f behaviors might interrupt that - it depends
// on if compiler truly inlines 'enqueue'
template <class TImpl = impl::reference_delegate_queue<> >
struct delegate_queue : TImpl
{
    static constexpr const char* TAG = "delegate_queue";

    typedef TImpl impl_type;

    estd::freertos::wrapper::ring_buffer buffer;

    using item_base = typename impl_type::item;

    delegate_queue(size_t sz)
    {
        buffer.create(sz, RINGBUF_TYPE_NOSPLIT);
    }

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
    // Code still in flux, args are flexibly and good practice, but confusing
    // impl configure_item is clumsy, breaks RAII, but easier to understand
    template <class F, class ...TArgs>
    inline BaseType_t enqueue(F&& f, TickType_t ticksToWait, TArgs&&...args)
    {
        typedef item<F> item_type;

        void* pvItem;

        if(buffer.send_acquire(&pvItem, sizeof(item_type), ticksToWait) == pdFALSE)
            return pdFALSE;

        ESP_LOGV(TAG, "enqueue: sz=%u", sizeof(item_type));

        item_type* i = new (pvItem) item_type(std::move(f), std::forward<TArgs>(args)...);

        impl_type::on_enqueue(i);

        return buffer.send_complete(pvItem);
    }

    // UNTESTED
    // Mainly for testing, the regular enqueue is likely preferable
    template <class F, class ...TArgs>
    inline BaseType_t enqueue_copy(F&& f, TickType_t ticksToWait, TArgs&&...args)
    {
        typedef item<F> item_type;

        item_type i(std::move(f), std::forward<TArgs>(args)...);

        impl_type::on_enqueue(&i);

        ESP_DRAM_LOGV(TAG, "enqueue_from_isr: sz=%u", sizeof(item_type));

        return buffer.send(&i, sizeof(item_type), ticksToWait);
    }

    // UNTESTED
    template <class F, class ...TArgs>
    inline BaseType_t enqueue_from_isr(F&& f, BaseType_t* pxHigherPriorityTaskWoke, TArgs&&...args)
    {
        typedef item<F> item_type;

        // DEBT: there's no isr xRingbufferSendAcquire variant that I could find, sure
        // wish there was
        item_type i(std::move(f), std::forward<TArgs>(args)...);

        impl_type::on_enqueue(&i);

        ESP_DRAM_LOGV(TAG, "enqueue_from_isr: sz=%u", sizeof(item_type));

        return buffer.send_from_isr(&i, sizeof(item_type), pxHigherPriorityTaskWoke);
    }

    template <class F>
    void dequeue(F&& f, TickType_t ticksToWait)
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive(&sz, ticksToWait);

        i->delegate();

        f(i);

        impl_type::on_dequeue(i);

        i->~item_assist();

        buffer.return_item(i);
    }

    void dequeue(TickType_t ticksToWait)
    {
        dequeue([](item_assist*){}, ticksToWait);
    }

    // UNTESTED
    void dequeue_from_isr(BaseType_t *pxHigherPriorityTaskWoken)
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive_from_isr(&sz);

        i->delegate();

        impl_type::on_dequeue(i);

        i->~item_assist();

        buffer.return_item_from_isr(i, pxHigherPriorityTaskWoken);
    }
};

}}
