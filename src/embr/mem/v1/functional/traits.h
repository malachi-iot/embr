#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class F, class Pool, Pool* pool>
struct innate_traits<mem::function<F, Pool, pool>>
{
    static constexpr bool unique = true;
    static constexpr bool shared = true;

    template <class Pool2, Pool2* pool2 = nullptr>
    using rebind_shared = mem::v1::shared_handle<mem::function<F, Pool2, pool2>, Pool2, pool2>;
};

}}

}}
