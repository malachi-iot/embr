#pragma once

#include <estd/cstddef.h>   // DEBT: bip/buffer should include this itself for 'byte'
#include <estd/internal/bip/buffer.h>
#include <estd/functional.h>

namespace embr { namespace experimental {

struct NoopMutex
{
    static constexpr bool lock_push() { return {}; }
    static constexpr bool unlock_push() { return {}; }
    static constexpr bool lock_pop() { return {}; }
    static constexpr bool unlock_pop() { return {}; }
};

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    class Mutex>
class ThunkBase : protected Mutex
{
    // Edge-case version which auto-invokes functor destructor immediately
    // after invocation
    using function_type = estd::detail::v2::function<
        void(void),
        estd::detail::impl::function_fnptr2_opt>;
    using model_base = typename function_type::model_base;

    // DEBT: It's possible an inline function flavor of this could do away
    // with intermediate model* since it knows in this context that model
    // ALWAYS is tacked on to the end
    struct Item
    {
        // Size of model portion only
        struct
        {
            // Functor max size:
            // * 4k on 16-bit (AVR)
            // * 512MB on 32-bit (ESP32)
            unsigned sz : (sizeof(unsigned) * 8) - 4;
            // Dormant, just for experimentation
            unsigned flag1 : 1;
        };

        char model[];

        static constexpr unsigned size(unsigned sz)
        {
            //return sizeof(sz) + sizeof(function) + sz;
            return offsetof(Item, model) + sz;
        }

        unsigned size() const
        {
            return size(sz);
        }
    };

    Mutex& mutex() { return *this; }

    Buf buf_;

    // We baked this right into function itself:
    // https://github.com/malachi-iot/estdlib/issues/39
    /*
    template <class F>
    class Wrapper
    {
        F f;

    public:
        constexpr Wrapper(F&& f) : f(std::move(f)) {}

        void operator()()
        {
            f();
            f.~F();
        }
    };  */

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(ThunkBase, buf_)

    // DEBT: See if we can find clever way to oerload and handle no-parameter flavor of F too
    template <class F, class Mutex2>
    bool enqueue(F&& f, Mutex2 mutex)
    {
        /*
        auto f2 = [f]
        {
            f();
            f.~F();
        };

        using F2 = decltype(f2); */

        //Wrapper<F> f2 { std::move(f) };
        //using F2 = Wrapper<F>;
        using F2 = F;

        //using inline_function = estd::experimental::inline_function<F, void(void*)>;
        using model_type = function_type::model<F2>;
        //int sz = sizeof(inline_function) + sizeof(Item);

        mutex.lock_push();

        // Make sure we have enough space
        if(buf_.unused() < sizeof(model_type))  return false;

        auto item = (Item*) buf_.offer_begin();

        // DEBT: I think I really want a move here, but C++ scolds me
        //function_type::model<F>(std::forward<F>(f));
        //new (b) inline_function(std::forward<F>(f));

        item->sz = sizeof(model_type);
        //model_type* m = new (item->model) model_type(std::move(f));
        new (item->model) model_type(std::forward<F>(f));
        //new (&item->function) function_type(m);

        buf_.offer_end(item->size());

        mutex.unlock_push();

        return true;
    }

    template <class F>
    bool enqueue(F&& f)
    {
        return enqueue(std::forward<F>(f), mutex());
    }

    bool empty() const
    {
        return buf_.used() == 0;
    }

    template <class Mutex2>
    bool invoke(Mutex2 mutex)
    {
        mutex.lock_pop();
        auto item = (Item*)buf_.peek();
        mutex.unlock_pop();

        if(item == nullptr) return false;
        auto model = (model_base*)item->model;

        // Models are directly invocable just like their function owners.
        // fnptr2_opt also calls functor destructor.
        (*model)();

        mutex.lock_pop();
        buf_.poll(item->size());
        mutex.unlock_pop();
        return true;
    }

    bool invoke()
    {
        return invoke(mutex());
    }
};

namespace layer1 {

template <unsigned N, class Mutex = NoopMutex>
using Thunk = ThunkBase<estd::layer1::bipbuf<N>, Mutex>;

}

namespace layer3 {

template <class Mutex = NoopMutex>
using Thunk = ThunkBase<estd::layer3::bipbuf, Mutex>;

}


}}
