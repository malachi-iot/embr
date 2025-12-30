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
    constexpr explicit unique_handle(handle_type handle, Pool* p = nullptr) :
        base_type(handle, p)
    {
    }

    unique_handle(const unique_handle&) = delete;

    ~unique_handle()
    {
        if(*this)   value()->ops().dealloc(handle_);
    }
};

}}

template <class T, class Pool, Pool* pool>
class unique_handle : public detail::unique_handle<Pool, pool>
{
    using base_type = detail::unique_handle<Pool, pool>;
    using typename base_type::handle_type;

public:
    constexpr explicit unique_handle(handle_type handle, Pool* p = nullptr) :
        base_type(handle, p) {}
};

}}
