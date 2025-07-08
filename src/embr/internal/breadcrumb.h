#pragma once

#include <estd/string.h>

// Reinterpretation of embr::coap 'triple' / URI mapper

namespace embr { namespace internal {

struct breadcrumb
{
    const char* name;
    const int id = -1;
    const int parent = -1;

    static ESTD_CPP_CONSTEVAL breadcrumb null()
    {
        return { nullptr };
    }
};

template <class T>
struct breadcrumb_traits
{
    static constexpr const char* name(const T& v) { return v.name; }
    static constexpr bool is_null(const T& v) { return v.name == nullptr; }
};

struct basic_searcher_base
{
    enum results
    {
        SEARCHING,
        MATCHED,        // 'crumbs' contains result
        NO_MATCH,
    };

    enum pred_result
    {
        DONE,
        PROCEED,
        FAST_FORWARD,   ///<! Grandchildren discovered, skip by these
    };
};

// For character by character affairs
// DEBT: This can be a template <class T, class Traits = searcher_traits> kind of thing
template <class T, class Traits = breadcrumb_traits<T>>
struct basic_searcher : basic_searcher_base
{
    using reference = const T&;
    using pointer = const T*;

    using traits = Traits;
    using char_type = const char;

    // Be sure to start crumb right after desired parent
    pointer crumbs_;
    int pos_ = 0;
    pointer marker_ = nullptr;

    explicit constexpr basic_searcher(pointer crumbs) : crumbs_{crumbs} {}

    bool match(char_type c)
    {
        const char* name = traits::name(*crumbs_);
        if(name[pos_] == c)
        {
            if(marker_ == nullptr)
            {
                // hi2u side effect
                marker_ = crumbs_;
            }
            return true;
        }

        return false;
    }

    // Pass in null termination also
    // DEBT: Need to upgrade predicate to return pass, fail or fast-forward.
    // Fast-forward is needed when the children being iterated over themselves have
    // children (want to fast forward over grandchildren etc)
    template <class Predicate>
    results search(char_type c, Predicate&& predicate)
    {
        if(match(c))
        {
            ++pos_;
            return c == 0 ? MATCHED : SEARCHING;
        }
        else
        {
            ++crumbs_;
            const int r = predicate(*crumbs_);
            // NOTE: Through the magic of implicit conversion, simpler scenarios may return a bool
            // in which case: true == proceed, false == complete
            if(r == DONE)
            {
                return NO_MATCH;
            }
            else if(marker_ == nullptr || r == FAST_FORWARD)
            {
                // If we had no semblance of a match so far, plunge forward

                // DEBT: Don't really want to do recursion, just convenient
                return search(c, std::forward<Predicate>(predicate));
            }
            else if(std::memcmp(crumbs_->name, marker_->name, pos_) == 0)
            {
                // If next crumb begins with same characters as last crumb,
                // then we're still in the game for searching. i.e:
                // 1. incoming key is 'hi2u'
                // 2. crumbs are: 'hi' and 'hi2u'
                // In that case marker has 'hi' in it and pos is 2

                // A limited while loop makes sense here to blast
                // through all near matches.  Clear out marker because we've
                // established him as a match, open up door for new marker
                marker_ = nullptr;

                // DEBT: Don't really want to do recursion, just convenient
                return search(c, std::forward<Predicate>(predicate));
            }

            // If movement to the next crumb doesn't match marker, then match
            // overall fails... fall through to NO_MATCH
        }

        return NO_MATCH;
    }

    results search(char_type c)
    {
        return search(c, [](reference v){ return traits::is_null(v) ? DONE : PROCEED; });
    }

    void reset()
    {
        marker_ = nullptr;
        pos_ = 0;
    }
};

using searcher = basic_searcher<breadcrumb>;

// Run to evaluate suitability of provided breadcrumb for matching
// DONE = specify no further searching
// PROCEED = valid place to look
// FAST_FORWARD = skip over this one
struct breadcrumb_functor
{
    using traits = breadcrumb_traits<breadcrumb>;

    const breadcrumb* const parent_;
    bool in_grandchild_ = false;
    const breadcrumb* last_ = nullptr;

    void reset()
    {
        in_grandchild_ = false;
        last_ = nullptr;
    }

    searcher::pred_result operator()(const breadcrumb& c)
    {
        if(traits::is_null(c))  return basic_searcher_base::DONE;

        if(c.parent == parent_->id)
        {
            last_ = &c;
            in_grandchild_ = false;
            return basic_searcher_base::PROCEED;
        }

        // Keep going until we notice we're back to parent->id matching, or otherwise all the way to the end
        // could be optimized, but hopefully slamming through all the remainders when no match is found isn't
        // too expensive

        if(in_grandchild_)
            return basic_searcher_base::FAST_FORWARD;
        if(last_ != nullptr && last_->id == c.parent)
        {
            // If last encountered breadcrumb is parent of this one, we're in child mode
            in_grandchild_ = true;
            return basic_searcher_base::FAST_FORWARD;
        }

        // not a child or grandchild, and not a candidate to become a grandchild
        return basic_searcher_base::DONE;
    }
};

inline const breadcrumb* search2(const breadcrumb* crumbs, const char* s)
{
    searcher srch{crumbs};
    breadcrumb_functor functor{crumbs};

    do
    {
        const searcher::results r = srch.search(*s, functor);
        switch(r)
        {
            case searcher::NO_MATCH: return nullptr;
            case searcher::MATCHED: return srch.marker_;
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
    for(;crumbs->name != nullptr && crumbs->parent == parent; ++crumbs)
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
