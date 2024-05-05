#pragma once

#include <estd/cstddef.h>   // DEBT: bip/buffer should include this itself for 'byte'
#include <estd/internal/bip/buffer.h>
#include <estd/functional.h>

namespace embr { namespace experimental {

//template <class Bipbuf>
class Thunk
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
        unsigned sz;
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



    estd::layer1::bipbuf<256> buf_;

    // NOTE: We're gonna bake this right into function itself:
    // https://github.com/malachi-iot/estdlib/issues/39
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
    };

public:
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

    void invoke()
    {
        auto item = (Item*)buf_.peek();

        if(item == nullptr) return;
        auto model = (model_base*)item->model;

        function_type f(model);
        f();

        // DEBT: Doesn't call dtor, but perhaps should.  Additionally, our thunk
        // behavior in fact prefers NOT to since we know invocation time is immediately
        // followed by dtor, so a kind of baked-in optimization.  Therefore, we'd
        // like specialized flavors of model with and without destruct capability
        // See https://github.com/malachi-iot/estdlib/issues/39
        //model->~model_base();

        //item->function(nullptr);

        buf_.poll(item->size());
    }
};

}}
