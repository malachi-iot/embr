// NOTE: Depends on esp-idf framework!  Also depends on c++11
// delegate_queue is entirely based on esp-idf's ring buffer.
// Since we might implement our own version of this ring buffer someday, putting
// delegate queue out in non-platform specific area, for the time being

#include <algorithm>    // For std::copy_n

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
    typedef estd::detail::function<void(void)> delegate_type;

    estd::freertos::wrapper::ring_buffer buffer;

    using item_base = typename impl_type::item;

    delegate_queue(size_t sz)
    {
        buffer.create(sz, RINGBUF_TYPE_NOSPLIT);
    }

    ~delegate_queue()
    {
        buffer.free();
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

    // EXPERIMENTAL
    // Probably will work well, but it may be more efficient to have item "types"
    // which know how to dequeue in special ways
    template <typename F>
    struct item_with_storage : item_base
    {
        struct F2
        {
            F f;
            const uint8_t* data;
            const unsigned sz;

            void operator()()
            {
                f(data, sz);
            }
        };

        estd::experimental::inline_function<F2, void(void)> delegate;

        template <class ...TArgs>
        item_with_storage(F&& f, unsigned sz, TArgs&&...args) :
            item_base(std::forward<TArgs>(args)...),
            delegate(F2{std::move(f), data, sz})
        {}

        uint8_t data[];
    };

    // Think of this as a brute force union
    struct item_assist : item_base
    {
        estd::detail::function<void(void)> delegate;

        template <class ...TArgs>
        item_assist(const delegate_type& f, TArgs&&...args) :
            item_base(std::forward<TArgs>(args)...),
            delegate(f)
        {}
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


    // EXPERIMENTAL
    template <class F>
    inline BaseType_t enqueue_with_storage(F&& f, TickType_t ticksToWait,
        const uint8_t* data, unsigned sz)
    {
        /*
        using f_type = f2_type<F>;
        using item_type = item_with_storage<f_type>; */
        using item_type = item_with_storage<F>;

        // DEBT: offsetof gets mad because item_type isn't a "standard type",
        // so doing this old school cheezy offset
        item_type* dummy = nullptr;
        unsigned o1 = (unsigned)&dummy->data;
        //unsigned o1 = offsetof(item_type, data);
        unsigned o2 = o1 + sz;  

        void* pvItem;

        if(buffer.send_acquire(&pvItem, o2, ticksToWait) == pdFALSE)
            return pdFALSE;

        item_type* i = new (pvItem) item_type(std::move(f), sz);

        // TODO: Put it in constructor
        //memcpy(i->data, data, sz);
        std::copy_n(data, sz, i->data);

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

    // BROKEN
    // Won't work because estd::detail::function can't be moved around
    template <class F, class ...TArgs>
    inline BaseType_t enqueue_from_isr_broken(F&& f, BaseType_t* pxHigherPriorityTaskWoke, TArgs&&...args)
    {
        typedef item<F> item_type;

        // DEBT: there's no isr xRingbufferSendAcquire variant that I could find, sure
        // wish there was.  See https://github.com/espressif/esp-idf/issues/10527
        item_type i(std::move(f), std::forward<TArgs>(args)...);

        impl_type::on_enqueue(&i);

        ESP_DRAM_LOGV(TAG, "enqueue_from_isr: sz=%u", sizeof(item_type));

        //return pdFALSE;
        return buffer.send_from_isr(&i, sizeof(item_type), pxHigherPriorityTaskWoke);
    }

    // EXPERIMENTAL
    // Works, but how useful is it really?
    template <class ...TArgs>
    inline BaseType_t enqueue_from_isr(const delegate_type& f, BaseType_t* pxHigherPriorityTaskWoke, TArgs&&...args)
    {
        typedef item_assist item_type;

        // DEBT: there's no isr xRingbufferSendAcquire variant that I could find, sure
        // wish there was (see https://github.com/espressif/esp-idf/issues/10527)
        item_type i(f, std::forward<TArgs>(args)...);

        impl_type::on_enqueue(&i);

        ESP_DRAM_LOGV(TAG, "enqueue_from_isr: sz=%u", sizeof(item_type));

        //return pdFALSE;
        return buffer.send_from_isr(&i, sizeof(item_type), pxHigherPriorityTaskWoke);
    }

    template <class F>
    BaseType_t dequeue(F&& run_after_delegate, TickType_t ticksToWait)
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive(&sz, ticksToWait);

        if(i == nullptr) return pdFALSE;

        i->delegate();

        run_after_delegate(i);

        impl_type::on_dequeue(i);

        i->~item_assist();

        buffer.return_item(i);

        return pdTRUE;
    }

    BaseType_t dequeue(TickType_t ticksToWait)
    {
        return dequeue([](item_assist*){}, ticksToWait);
    }

    // UNTESTED
    BaseType_t dequeue_from_isr(BaseType_t *pxHigherPriorityTaskWoken)
    {
        size_t sz;
        auto i = (item_assist*)buffer.receive_from_isr(&sz);

        if(i == nullptr) return pdFALSE;

        i->delegate();

        impl_type::on_dequeue(i);

        i->~item_assist();

        buffer.return_item_from_isr(i, pxHigherPriorityTaskWoken);

        return pdTRUE;
    }

    // DEBT: copy/paste from estd::experimental::function - some kind of general factory would be better
    // DEBT: need to enforce RVO rules more
    template <typename F>
    ESTD_CPP_CONSTEXPR_FUNCTION static estd::experimental::inline_function<F, void(void)> make_inline(F&& f)
    {
        return estd::experimental::inline_function<F, void(void)>(std::move(f));
    }
};

}}

