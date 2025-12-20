#pragma once

#include <estd/iterator.h>
#include <estd/internal/value_evaporator.h>

namespace embr { namespace mem { inline namespace v1 {

// DEBT: Combine with unit_base CRTP of the same stripe
template <class Derived>
class iterator_ops
{
public:
    Derived operator++(int)
    {
        const Derived& self = static_cast<const Derived&>(*this);
        Derived copied(self);

        copied.operator++();

        return copied;
    }

    Derived operator--(int)
    {
        const Derived& self = static_cast<const Derived&>(*this);
        Derived copied(self);

        copied.operator--();

        return copied;
    }
};

// Boost-style
// DEBT: Consolidate with old estd::experimental::filter_iterator, use EBO and
// hopefully refine 'evaporator' a bit along the way
template <class Pred, class It>
class filter_iterator :
    public estd::internal::struct_evaporator<Pred>,
    public iterator_ops<filter_iterator<Pred, It>>
{
    using base_type = estd::internal::struct_evaporator<Pred>;

    // DEBT: Add is_default_constructible to estd, and perhaps a
    // functor_evaporator for all this
    using pred_constructible = std::is_default_constructible<Pred>;
    static constexpr Pred* pred = nullptr;

    It current_;

    using this_type = filter_iterator;
    using iterator = It;
    using traits = estd::iterator_traits<iterator>;

    // At the moment, 0-size functor is required
    constexpr bool predicate(const iterator& v) const
    {
        //return base_type::value()(*v);
        // NOTE: Total trick - in cases of a non-capturing lambda
        // (or otherwise is_empty functor) we skip default construction, since that
        // is missing for a lambda.  It's conceivable an is_empty functor might actually
        // have a constructor, so we need to account for that
        return pred->operator()(*v);
    }

public:
    //ESTD_CPP_STD_VALUE_TYPE(typename traits::value_type)
    using value_type = typename traits::value_type;
    using reference = typename traits::reference;
    using pointer = typename traits::pointer;
    using const_reference = const value_type&;

    constexpr filter_iterator(iterator it) : current_{it}
    {
        while(predicate(current_) == false) ++current_;
    }

    this_type& operator++()
    {
        do
        {
            ++current_;
        }
        while(predicate(current_) == false);

        return *this;
    }

    reference operator*() { return *current_; }
    constexpr const_reference operator*() const { return *current_; }
    pointer operator->() { return &(*current_); }
};



}}}

