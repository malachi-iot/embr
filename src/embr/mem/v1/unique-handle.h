#include "fwd.h"
#include "lock-guard.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Pool, Pool* pool>
class unique_handle : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;
    using base_type::value;
    using base_type::is_global;
    using base_type::handle_;
    using handle_type = typename Pool::handle_type;

public:
    constexpr unique_handle(handle_type handle, Pool* p) :
        base_type(handle, p)
    {
        value()->ops().ref_up(handle_);
    }

    constexpr unique_handle(unique_handle&& move_from) noexcept:
        base_type(move_from)
    {
        move_from.reset();
    }

    unique_handle(const unique_handle&) = delete;
};

}}

}}
