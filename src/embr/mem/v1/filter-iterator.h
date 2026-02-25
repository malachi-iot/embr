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
        auto& self = static_cast<Derived&>(*this);
        Derived copied(self);

        self.operator++();

        return copied;
    }

    Derived operator--(int)
    {
        auto& self = static_cast<Derived&>(*this);
        Derived copied(self);

        self.operator--();

        return copied;
    }
};

template <class Derived>
struct iterator_base_ops
{
    constexpr bool operator==(const Derived& compare_to) const
    {
        auto& self = static_cast<const Derived&>(*this);
        return self.base() == compare_to.base();
    }

    constexpr bool operator!=(const Derived& compare_to) const
    {
        auto& self = static_cast<const Derived&>(*this);
        return self.base() != compare_to.base();
    }
};


// EXPERIMENTAL
template <class Pred, class It>
struct generic_end_predicate
{
    const It end_;

    template <class T>
    constexpr bool operator()(T v) const
    {
        if(v == end_)   return true;

        return Pred{}(v);
    }
};


// Boost-style
// DEBT: Consolidate with old estd::experimental::filter_iterator, use EBO and
// hopefully refine 'evaporator' a bit along the way
template <class Pred, class It>
class filter_iterator :
    public estd::internal::struct_evaporator<Pred>,
    public iterator_base_ops<filter_iterator<Pred, It>>,
    public iterator_ops<filter_iterator<Pred, It>>
{
    using ops = iterator_ops<filter_iterator<Pred, It>>;
    using base_ops = iterator_base_ops<filter_iterator<Pred, It>>;
    using base_type = estd::internal::struct_evaporator<Pred>;

    // DEBT: Add is_default_constructible to estd, and perhaps a
    // functor_evaporator for all this
    using pred_constructible = std::is_default_constructible<Pred>;
    static constexpr Pred* pred_null = nullptr;
    using is_pred_empty = typename estd::is_empty<Pred>::type;

    It current_;

    using this_type = filter_iterator;
    using iterator = It;
    using traits = estd::iterator_traits<iterator>;

    constexpr bool predicate(const iterator& v, estd::false_type) const
    {
        return base_type::value()(*v);
    }

    constexpr bool predicate(const iterator& v, estd::true_type /* is_empty */) const
    {
        //return base_type::value()(*v);
        // NOTE: Total trick - in cases of a non-capturing lambda
        // (or otherwise is_empty functor) we skip default construction, since that
        // is missing for a lambda.  It's conceivable an is_empty functor might actually
        // have a constructor, so we need to account for that
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Wnull-dereference"

        return pred_null->operator()(*v);

#pragma GCC diagnostic pop
    }

public:
    //ESTD_CPP_STD_VALUE_TYPE(typename traits::value_type)
    using value_type = typename traits::value_type;
    using reference = typename traits::reference;
    using pointer = typename traits::pointer;
    using const_reference = const value_type&;

    using ops::operator ++;
    using ops::operator --;
    using base_ops::operator ==;
    using base_ops::operator !=;

    ESTD_CPP_CONSTEXPR(20) filter_iterator(iterator it) : current_{it}
    {
        int p;
        while((p = predicate(current_, is_pred_empty{}) == false)) ++current_;
    }

    constexpr filter_iterator(Pred&& pred, iterator it) :
        base_type(std::move(pred)),
        current_{it}
    {
        while(predicate(current_, is_pred_empty{}) == false) ++current_;
    }

    this_type& operator++()
    {
        int p;

        // Increment until predicate is satisfied
        do
        {
            ++current_;
        }
        while((p = predicate(current_, is_pred_empty{})) == false);

        return *this;
    }

    reference operator*() { return *current_; }
    constexpr const_reference operator*() const { return *current_; }
    pointer operator->() { return &(*current_); }

    // Boost style
    constexpr const iterator& base() const { return current_; }

    constexpr bool operator==(const iterator& compare_to)
    {
        return current_ == compare_to;
    }

    constexpr bool operator!=(const iterator& compare_to)
    {
        return current_ != compare_to;
    }
};

// <= c++14 needs this
template <class Pred, class It>
constexpr Pred* filter_iterator<Pred, It>::pred_null;

}}}

