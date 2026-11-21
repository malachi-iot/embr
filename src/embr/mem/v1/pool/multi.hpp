#include <estd/utility.h>

#include "../../../internal/mutex.h"

#include "fwd.h"

namespace embr { namespace mem { inline namespace v1 {

template <class It, class End, class F, class Mutex = embr::internal::noop_mutex>
EMBR_GC_INLINE_FLATTEN void multi_op(It begin, End end, F&& f, Mutex&& mutex = {})
{
    mutex.lock();

    for(It i = begin; i < end; ++i) f(*i);

    mutex.unlock();
}

// DEBT: It and End subject to easy errors
// 04NOV26 MB - See if we can consolidate this style with https://en.cppreference.com/cpp/thread/lock,
// probably not because we need a runtime begin/end but worth thinking about anyway
template <class Traits, class It, class End, class Mutex>
void multi_lock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex&& mutex)
{
    using handle_type = typename Traits::handles_type::handle_type;
    using type = detail::v1::sparse_handle<void, handle_type>;

    multi_op(begin, end, [&](type v)
    {
        ops.lock(v.handle());
    }, std::forward<Mutex>(mutex));
}


// DEBT: It and End subject to easy errors
template <class Traits, class It, class End, class Mutex>
EMBR_GC_INLINE void multi_unlock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex&& mutex)
{
    using handle_type = typename Traits::handles_type::handle_type;
    using type = detail::v1::sparse_handle<void, handle_type>;

    multi_op(begin, end, [&](type v)
    {
        ops.unlock(v.handle());
    }, mutex);
}

}}}
