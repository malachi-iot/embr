// Rebaser to ensure scheduler doesn't overflow
// NOTE: Goal is portability - in the short term we'll put this in esp-idf category
#pragma once

// DEBT: Move this elsewhere
#define EMBR_CPP_VALUE_TYPE(T) \
typedef T value_type;           \
typedef value_type& reference;  \
typedef const value_type& const_reference; \
typedef T* pointer;


namespace embr { namespace internal {

template <class T>
struct rebase_traits
{
    EMBR_CPP_VALUE_TYPE(T)

    typedef typename value_type::time_point duration;

    inline static void rebase(reference v, duration t) { v.rebase(t); }
};


template <class TContainer, class TTraits = rebase_traits<typename TContainer::value_type> >
class Rebaser
{
    typedef TContainer container_type;
    typedef typename container_type::iterator iterator;

    typedef typename container_type::value_type value_type;
    typedef TTraits traits_type;
    typedef typename traits_type::duration duration;

    container_type& container_;

public:
    Rebaser(container_type& container) : container_(container)
    {

    }

    void rebase(duration d)
    {
        iterator i = container_.begin();

        for(;i != container_.end(); ++i)
        {
            value_type& v = *i;

            traits_type::rebase(v, d);
        }
    }
};

}}