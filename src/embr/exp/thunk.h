#pragma once

#include <estd/cstddef.h>   // DEBT: bip/buffer should include this itself for 'byte'
#include <estd/internal/bip/buffer.h>
#include <estd/functional.h>

namespace embr { namespace experimental {

//template <class Bipbuf>
class Thunk
{
    using function_type = estd::detail::function<void(void*)>;

    // DEBT: It's possible an inline function flavor of this could do away
    // with intermediate model* since it knows in this context that model
    // ALWAYS is tacked on to the end
    struct Item
    {
        // Size of model portion only
        unsigned sz;
        function_type function;
        uint8_t model[];

        /*
        template <class F>
        Item(unsigned sz, F&& f) :
            sz(sz),
        {
            using inline_function = estd::experimental::inline_function<F, void(void*)>;

        }   */

        unsigned size() const
        {
            //return sizeof(sz) + sizeof(function) + sz;
            return sizeof(Item) + sz;
        }
    };



    estd::layer1::bipbuf<256> buf_;

public:
    template <class F>
    void enqueue(F&& f)
    {
        using inline_function = estd::experimental::inline_function<F, void(void*)>;
        using model_type = function_type::model<F>;
        //int sz = sizeof(inline_function) + sizeof(Item);

        // Need to make sure we have enough space
        //if(buf_.unused())
        auto b = buf_.offer_begin();
        auto item = (Item*) b;
        //item->sz = sz;

        //new (b) Item { sz, }

        // DEBT: I think I really want a move here, but C++ scolds me
        //function_type::model<F>(std::forward<F>(f));
        //new (b) inline_function(std::forward<F>(f));

        item->sz = sizeof(model_type);
        model_type* m = new (item->model) model_type(std::forward<F>(f));
        new (&item->function) function_type(m);

        buf_.offer_end(item->size());
    }

    void invoke()
    {
        auto item = (Item*)buf_.peek();

        if(item == nullptr) return;

        item->function(nullptr);

        buf_.poll(item->size());
    }
};

}}