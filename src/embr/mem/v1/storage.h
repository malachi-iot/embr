#pragma once

#include <estd/internal/macros.h>
#include <estd/internal/size.h>

#include "unit.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
class storage : public Traits
{
    using this_type = storage;

public:
    using traits = Traits;
    using block_type = typename traits::block;
    using container_type = typename traits::type;

    // Doesn't work for span, see https://github.com/malachi-iot/estdlib/issues/167
    using iterator_traits = estd::iterator_traits<container_type>;

protected:
    template <class Traits2>
    friend class pool_ops;

    container_type pool_;

    ESTD_CPP_CONSTEXPR(14) block_type* block(const bytes_unit<unsigned>& at)
    {
        //assert(at.count() != page_type::null);
        return reinterpret_cast<block_type*>(estd::data(pool_) + at.count());
    }

    ESTD_CPP_CONSTEXPR(14) const block_type* block(const bytes_unit<unsigned>& at) const
    {
        return reinterpret_cast<const block_type*>(estd::data(pool_) + at.count());
    }

public:
    template <class ...Args>
    explicit constexpr storage(Args&&...args) :
        pool_(std::forward<Args>(args)...)
    {
        // DEBT: Unhardcode this guy, though may be difficult since 'page' isn't specified yet
        constexpr unsigned aliasing = sizeof(void*);

        assert(estd::size(pool_) % aliasing == 0);
    }

    // Just for diagnostics
    const char* data() const { return estd::data(pool_); }
};

}}

}}
