#include <estd/functional.h>

#include "../lock-handle.h"

#include "fwd.h"

namespace embr { namespace mem {

// DEBT: Consider always making him shared_handle.  Briefly did that, but it occurs that very tight constraint
// environments may have shared counter disabled (see block_6).  Counterpoint is unique_handle behaves much more
// like std::function
template <class R, class ...Args, class Pool, Pool* pool,
    template <class, estd::detail::impl::fn_options> class Impl>
class function<R(Args...), Pool, pool, Impl> : public detail::v1::unique_handle<Pool, pool>
{
    using base_type = detail::v1::unique_handle<Pool, pool>;
    using typename base_type::handle_type;

#if UNIT_TESTING
public:
#endif
    using sparse_model = detail::v1::sparse_model<R(Args...), handle_type, Impl>;

public:
    constexpr function(Pool* pool2, estd::nullptr_t) :
        base_type(base_type::null, pool2) {}
    constexpr function(estd::nullptr_t) : base_type(base_type::null) {} // NOLINT

    template <class F2>
    constexpr function(Pool* pool2, F2&& f) :
        base_type(sparse_model::make(pool2, std::forward<F2>(f)).handle(), pool2)
    {
    }

    template <class F2>
    constexpr explicit function(F2&& f) :   // NOLINT
        base_type(sparse_model::make(pool, std::forward<F2>(f)).handle())
    {
        static_assert(pool != nullptr);
    }

    constexpr R operator()(Args&&...args) const
    {
        return sparse_model::invoke(base_type::pool_(), base_type::handle_, std::forward<Args>(args)...);
    }
};

}}
