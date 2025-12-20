#pragma once

#include <estd/internal/rtto.h>

#include "enum.h"
#include "fwd.h"
#include "page.h"


namespace embr { namespace mem {

namespace detail { inline namespace v1 {

class alignas(void*) block : public block_mode_base
{
protected:
    struct alignas(void*)
    {
        unsigned prev_ : 8;
        unsigned next_ : 8;
        modes mode_ : 2;
        bool allocated_: 1;
    };

    char data_[];

public:
    block() = default;
    explicit block(modes mode, bool allocated) :
        mode_{mode},
        allocated_{allocated}
    {}

    // emplace constructors
    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::Immobile>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::Trivial>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::Rtto>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::RttoBase>, estd::in_place_type_t<T>, Args&&...args);

    block& operator=(const block&) = default;

    constexpr unsigned next() const { return next_; }
    constexpr bool allocated() const { return allocated_; }

    void* data() { return data_; }

    template <modes mode>
    static constexpr unsigned header_size()
    {
        return mode == Trivial ? sizeof(block) : (sizeof(block) + sizeof(estd::internal::rtto_base::base));
    }
};

#if UNIT_TESTING
/*
struct block_exposer : block
{
    using block::data_;
};

static_assert(offsetof(block_exposer, data_) == sizeof(void*));*/
#endif


// EXPERIMENTAL
class alignas(void*) small_block : block_mode_base
{
    struct alignas(void*)
    {
        unsigned prev_ : 6;
        modes mode_ : 2;
        unsigned next_ : 6;
        bool allocated_: 1;
    };
public:
};

}}

}}
