#pragma once

#include <estd/cstddef.h>   // DEBT: bip/buffer should include this itself for 'byte'
#include <estd/internal/bip/buffer.h>
#include <estd/functional.h>

namespace embr { namespace experimental {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf>
class ThunkBase
{
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
            // Functor bigger than 64k?  I don't think so!
            unsigned sz : 16;
            // Dormant, just for experimentation
            unsigned flag1 : 1;
        };

        //function_type function;
        uint8_t model[];

        /*
        template <class F>
        Item(unsigned sz, F&& f) :
            sz(sz),
        {
            using inline_function = estd::experimental::inline_function<F, void(void*)>;

        }   */

        static unsigned size(unsigned sz)
        {
            //return sizeof(sz) + sizeof(function) + sz;
            return offsetof(Item, model) + sz;
        }

        unsigned size() const
        {
            return size(sz);
        }
    };



    //estd::layer1::bipbuf<256> buf_;
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
    template <class F>
    bool enqueue(F&& f)
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

        return true;
    }

    bool empty() const
    {
        return buf_.used() == 0;
    }

    void invoke()
    {
        auto item = (Item*)buf_.peek();

        if(item == nullptr) return;
        auto model = (model_base*)item->model;

        // Models are directly invocable just like their function owners.
        // fnptr2_opt also calls functor destructor.
        (*model)();

        buf_.poll(item->size());
    }
};

using Thunk = ThunkBase<estd::layer1::bipbuf<256>>;
using Thunk2 = ThunkBase<estd::layer3::bipbuf>;

}}
