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
};

// For character by character affairs
// DEBT: This can be a template <class T, class Traits = searcher_traits> kind of thing
template <class T, class Traits = breadcrumb_traits<T>>
struct basic_searcher
{
    using pointer = const T*;
    using traits = Traits;

    // Be sure to start crumb right after desired parent
    pointer crumbs_;
    int pos = 0;
    pointer marker_ = nullptr;

    bool match(char c)
    {
        const char* name = traits::name(*crumbs_);
        if(name[pos] == c)
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

    enum results
    {
        SEARCHING,
        MATCHED,        // 'crumbs' contains result
        NO_MATCH,
    };

    enum pred_result
    {
        OK,
        FAST_FORWARD,
        DONE
    };

    // Pass in null termination also
    // DEBT: Need to upgrade predicate to return pass, fail or fast-forward.
    // Fast-forward is needed when the children being iterated over themselves have
    // children (want to fast forward over grandchildren etc)
    template <class Predicate>
    results search(char c, Predicate&& predicate)
    {
        if(match(c))
        {
            ++pos;
            return c == 0 ? MATCHED : SEARCHING;
        }
        else
        {
            ++crumbs_;
            if(predicate(*crumbs_) == false)
            {
                return NO_MATCH;
            }
            else if(marker_ == nullptr)
            {
                // If we had no semblance of a match so far, plunge forward

                // DEBT: Don't really want to do recursion, just convenient
                return search(c, std::forward<Predicate>(predicate));
            }
            else if(std::memcmp(crumbs_->name, marker_->name, pos) == 0)
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
};

using searcher = basic_searcher<breadcrumb>;

inline const breadcrumb* search2(const breadcrumb* crumbs, const char* s)
{
    const int parent_id = crumbs->parent;

    searcher srch{crumbs};

    do
    {
        const searcher::results r = srch.search(*s, [parent_id](const breadcrumb& c)
        {
            return c.parent == parent_id;
        });
        switch(r)
        {
            case searcher::NO_MATCH: return nullptr;
            case searcher::MATCHED: return srch.marker_;
            default: break;
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