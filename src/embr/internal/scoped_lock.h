#pragma once

#include <estd/type_traits.h>

namespace embr {

namespace internal {

// TODO: Move this out to estd
// TODO: Document that this is tuned for an acquirable resource lock
// DEBT: Consider a name change to lock_guard
// DEBT: Consider upgrading it to detect lightweight
// guard-style locking (such as mutex) vs resource acquisition guarding (done here, memory locking)
// One could probably do this by looking for the presence of 'value_type'
template <class TLockable, bool const_lock = false, class enabled = void>
class scoped_lock;

template <class TLockable>
class scoped_lock<TLockable, false>
{
    TLockable lockable;

public:
    typedef typename TLockable::value_type value_type;

private:
    value_type& value;

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    scoped_lock(TLockable&& lockable) :
        lockable(std::move(lockable)),
        value(lockable.lock())
    {
    }
#endif

    scoped_lock(TLockable& lockable) :
        lockable(lockable),
        value(lockable.lock())
    {
    }

    ~scoped_lock()
    {
        lockable.unlock();
    }

    value_type& operator*() { return value; }
};

}

}