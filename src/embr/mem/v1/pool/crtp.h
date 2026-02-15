#pragma once

#include <estd/mutex.h>

#include "../block.h"
#include "../unit.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

#pragma push_macro("THIS")
#pragma push_macro("OPS")
#undef THIS
#undef OPS
#define THIS static_cast<Derived*>(this)
#define OPS THIS->ops()


template <class Derived>
class pool_crtp
{
    //using handle_type = typename Derived::handle_type;

public:
    bool realloc(int h, unsigned size)
    {
        auto bn = OPS.get_bundle(h);
        // DEBT: remove explicit bytes_unit, dependent on https://github.com/malachi-iot/estdlib/issues/173
        return OPS.realloc(bn, bn.header_size() + bytes_unit<unsigned>(size));
    }

    void reset()
    {
        OPS.reset();
    }

    unsigned allocated()
    {
        return OPS.alloced().count();
    }

    // Higher thresh = demand quicker, easier GC
    int gc(int thresh = 0)
    {
        using ops_type = typename Derived::ops_type;
        using fragmentation = typename ops_type::fragmentation;
        fragmentation frag;
        const typename fragmentation::candidate& cand = frag.candidates[0];

        OPS.assess(&frag);

        if(cand.score > 0)
        {
            if(cand.score > thresh)
                OPS.defrag(cand);
        }

        return cand.score;
    }
};


// DEBT: I was resisting an ebo/mutex() paradigm but that's starting to look like
// the best option
template <class Derived, class Mutex = void>
class pool_mutex_crtp //: public pool_crtp<Derived>
{
    using base_type = pool_crtp<Derived>;

protected:
    Mutex mutex_;

    template <class ...Args>
    constexpr explicit pool_mutex_crtp(estd::in_place_type_t<Mutex>, Args&&...args) :
        mutex_{std::forward<Args>(args)...}
    {

    }

    constexpr pool_mutex_crtp() = default;

public:
    using mutex_type = Mutex;

    template <class Handle>
    void* lock(Handle h)
    {
        return OPS.lock(h, mutex_);
    }

    template <class Handle>
    void unlock(Handle h)
    {
        OPS.unlock(h, mutex_);
    }


    // Only for trivial mode, use construct otherwise
    template <class Derived2 = Derived>
    typename Derived2::handle_type alloc(unsigned sz)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        constexpr auto mode = v1::block_mode_enum::Trivial;
        // DEBT: Uncouple from block_8
        constexpr unsigned block_sz = v1::block_8::header_size(mode).count();
        // DEBT: Probably do_alias ought to be in page_type
        return OPS.alloc(OPS.do_alias(sz + block_sz), mode).handle;
    }


    template <class T, class ...Args, class Derived2 = Derived>
    typename Derived2::handle_type construct(Args&&...args)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        return detail::construct<T>(OPS.storage_, OPS.handles_, std::forward<Args>(args)...);
    }


    void dealloc(int h)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        OPS.dealloc(h);
    }

    // Higher thresh = demand quicker, easier GC
    int gc(int thresh = 0)
    {
        estd::lock_guard<Mutex> lg(mutex_);

        return base_type::gc(thresh);
    }
};

template <class Derived>
class pool_mutex_crtp<Derived> //: public pool_crtp<Derived>
{
public:
    using mutex_type = void;

    template <class Handle>
    void* lock(Handle h)
    {
        return OPS.lock(h);
    }

    template <class Handle>
    void unlock(Handle h)
    {
        OPS.unlock(h);
    }


    template <class T, class ...Args, class Derived2 = Derived>
    typename Derived2::handle_type construct(Args&&...args)
    {
        return detail::construct<T>(OPS.storage_, OPS.handles_, std::forward<Args>(args)...);
    }

    // Only for trivial mode, use construct otherwise
    template <class Derived2 = Derived>
    typename Derived2::handle_type alloc(unsigned sz)
    {
        constexpr auto mode = v1::block_mode_enum::Trivial;
        // DEBT: Uncouple from block_8
        constexpr unsigned block_sz = v1::block_8::header_size(mode).count();
        return OPS.alloc(OPS.do_alias(sz + block_sz), mode).handle;
    }


    void dealloc(int h)
    {
        OPS.dealloc(h);
    }


    /*
    template <class Handle>
    void realloc(Handle h, unsigned sz)
    {
        // FIX: Not complete
        OPS.realloc(OPS.get_bundle(h), OPS.do_alias(sz));
    }   */
};



#pragma pop_macro("OPS")
#pragma pop_macro("THIS")

}}

}}
