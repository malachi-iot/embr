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

// For character by character affairs
// DEBT: This can be a template <class T, class Traits = searcher_traits> kind of thing
struct searcher
{
    using pointer = const breadcrumb*;

    // Be sure to start crumb right after desired parent
    pointer crumbs_;
    int pos;
    pointer marker_ = nullptr;

    bool match(char c)
    {
        if(crumbs_->name[pos] == c)
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

    // Pass in null termination also
    results search(char c, int parent_id)
    {
        if(match(c))
        {
            ++pos;
            if(c == 0)
            {
                return MATCHED;
            }

            return SEARCHING;
        }
        else
        {
            ++crumbs_;
            if(parent_id != crumbs_->parent)
            {
                return NO_MATCH;
            }
            else if(marker_ == nullptr)
            {
                // If we had no semblance of a match so far, plunge forward

                // DEBT: Don't really want to do recursion, just convenient
                return search(c, parent_id);
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
                return search(c, parent_id);
            }

            // If movement to the next crumb doesn't match marker, then match
            // overall fails... fall through to NO_MATCH
        }

        return NO_MATCH;
    }

    results search(const char* s)
    {
        const int parent_id = crumbs_->parent;

        while(*s != 0)
        {
            results r = search(*s++, parent_id);
            if(r != SEARCHING) return r;
        }

        return search(*s, parent_id);
    }
};

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