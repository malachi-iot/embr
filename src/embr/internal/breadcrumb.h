#pragma once

#include <estd/string.h>
#include <estd/string_view.h>

// Reinterpretation of embr::coap 'triple' / URI mapper

namespace embr { namespace internal {

struct breadcrumb
{
    const estd::string_view name;
    const int id = -1;
    const int parent = -1;

    static ESTD_CPP_CONSTEVAL breadcrumb null()
    {
        return { {} };
    }
};

template <class T>
struct breadcrumb_traits
{
    static constexpr const estd::string_view& name(const T& v) { return v.name; }
    static constexpr bool is_null(const T& v) { return v.name.empty(); }
};

struct breadcrumb_searcher_base
{
    enum results
    {
        SEARCHING,
        MATCHED,        // 'crumbs' contains result
        NO_MATCH,
    };

    enum pred_result
    {
        DONE,           ///<! End of candidates reached
        PROCEED,
        FAST_FORWARD,   ///<! Grandchildren discovered, skip by these
    };
};

// For character by character affairs
template <class T, class Traits = breadcrumb_traits<T>>
struct breadcrumb_searcher : breadcrumb_searcher_base
{
    using reference = const T&;
    using pointer = const T*;

    using traits = Traits;
    using char_type = const char;

private:
    // Be sure to start crumb right after desired parent - i.e. at very start of candidate list
    pointer crumbs_;
    int pos_ = 0;   // character position
    pointer marker_ = nullptr;

    // Compare crumbs_.name[pos] == c
    bool match(char_type c)
    {
        const estd::string_view& name = traits::name(*crumbs_);

        // If we're at the end, return true if null terminations match
        if(pos_ >= name.length()) return c == 0 && pos_ == name.length();

        if(name[pos_] != c) return false;

        // If we're matching so far, mark our current best candidate
        if(marker_ == nullptr)  marker_ = crumbs_;

        return true;
    }

    static bool match_prefix(const estd::string_view& eval, const estd::string_view& matched, unsigned len)
    {
        return len <= eval.length() && len <= matched.length() &&
            (std::memcmp(eval.data(), matched.data(), len) == 0);
    }

public:
    constexpr pointer marker() const { return marker_; }
    constexpr pointer crumbs() const { return crumbs_; }

    // DEBT: Only json decoder::context needs this default constructor, otherwise it really
    // shouldn't be exposed
    breadcrumb_searcher() = default;

    explicit constexpr breadcrumb_searcher(pointer crumbs) : crumbs_{crumbs} {}

    ///
    /// Pass in null termination also
    /// Fast-forward is needed when the children being iterated over themselves have
    /// children (want to fast forward over grandchildren etc)
    /// @param c
    /// @param predicate signature of f(breadcrumb) -> pred_result
    /// @return
    ///
    template <class Predicate>
    results search(char_type c, Predicate&& predicate)
    {
        while(!match(c))
        {
            int r;

            do
            {
                ++crumbs_;
            }
            while((r = predicate(*crumbs_)) == FAST_FORWARD);

            if(r == DONE)
            {
                return NO_MATCH;
            }
            else if(marker_ == nullptr)
            {
                // If we had no semblance of a match so far, plunge forward
            }
            else if(match_prefix(crumbs_->name, marker_->name, pos_))
            {
                // If next crumb begins with same characters as last crumb (up to pos_),
                // then we're still in the game for searching. i.e:
                // 1. incoming key is 'hi2u'
                // 2. crumbs are: 'hi' and 'hi2u'
                // In that case marker has 'hi' in it and pos is 2

                // Clear out marker because we've
                // established him as a match, open up door for new marker
                marker_ = nullptr;
            }
            else
                // If movement to the next crumb doesn't match marker, then match
                // overall fails...
                return NO_MATCH;
        }

        ++pos_;
        return c == 0 ? MATCHED : SEARCHING;
    }

    // DEBT: A little too convenient perhaps put this elsewhere
    results search(char_type c)
    {
        return search(c, [](reference v){ return traits::is_null(v) ? DONE : PROCEED; });
    }

    void reset(pointer crumbs)
    {
        crumbs_ = crumbs;
        marker_ = nullptr;
        pos_ = 0;
    }
};

// Run to evaluate suitability of provided breadcrumb for matching.  Does not do character/string matching
// DONE = specify no further searching
// PROCEED = valid place to look
// FAST_FORWARD = skip over this one
struct breadcrumb_functor
{
    using traits = breadcrumb_traits<breadcrumb>;
    using searcher = breadcrumb_searcher<breadcrumb>;

    // Not specifying 'parent' since that's awkward for root level nodes (parent is nullptr)
    // This also better aligns with init of searcher itself
    const breadcrumb* first_;
    bool in_grandchild_ = false;
    const breadcrumb* prev_ = nullptr;

    constexpr explicit breadcrumb_functor(const searcher* parent) : first_{parent->crumbs()}   {}

    void reset()
    {
        in_grandchild_ = false;
        prev_ = nullptr;
    }

    void reset(const breadcrumb* first)
    {
        first_ = first;
        in_grandchild_ = false;
        prev_ = nullptr;
    }

    searcher::pred_result operator()(const breadcrumb& c)
    {
        if(traits::is_null(c))  return searcher::DONE;

        if(c.parent == first_->parent)
        {
            prev_ = &c;
            in_grandchild_ = false;
            return searcher::PROCEED;
        }

        // Keep going until we notice we're back to parent->id matching, or otherwise all the way to the end
        // could be optimized, but hopefully slamming through all the remainders when no match is found isn't
        // too expensive

        if(in_grandchild_)
            return searcher::FAST_FORWARD;
        if(prev_ != nullptr && prev_->id == c.parent)
        {
            // If last encountered breadcrumb is parent of this one, we're in child mode
            in_grandchild_ = true;
            return searcher::FAST_FORWARD;
        }

        // not a child or grandchild, and not a candidate to become a grandchild
        return searcher::DONE;
    }
};

inline const breadcrumb* search2(const breadcrumb* crumbs, const char* s)
{
    using searcher = breadcrumb_searcher<breadcrumb>;
    searcher srch{crumbs};
    breadcrumb_functor functor{&srch};

    do
    {
        const searcher::results r = srch.search(*s, functor);
        switch(r)
        {
            case searcher::NO_MATCH: return nullptr;
            case searcher::MATCHED: return srch.marker();
            case searcher::SEARCHING: break;
        }
    }
    while(*s++ != 0);

    // Get here when crumbs has a longer match than us.  Be sure to put shorter matchers
    // first as part of hand-sort
    return nullptr;
}

template <class Impl>
const breadcrumb* search(const breadcrumb* crumbs,
    const estd::detail::basic_string<Impl>& name,
    int parent = -1)
{
    for(;!crumbs->name.empty() && crumbs->parent == parent; ++crumbs)
    {
        if(name == crumbs->name) return crumbs;
    }

    return crumbs;
}

inline const breadcrumb* search(const breadcrumb* crumbs,
    const char* name,
    int parent = -1)
{
    return search(crumbs, estd::layer2::const_string(name), parent);
}


}}

namespace embr { namespace breadcrumb { inline namespace v1 {

using breadcrumb = embr::internal::breadcrumb;
using functor = embr::internal::breadcrumb_functor;
using searcher = embr::internal::breadcrumb_searcher<breadcrumb>;

}}}
