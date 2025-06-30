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

    // Be sure to position with desired 'parent'
    pointer crumbs;
    int pos;
    pointer marker = nullptr;

    bool match(char c)
    {
        if(crumbs->name[pos] == c)
        {
            if(marker == nullptr)
            {
                // hi2u side effect
                marker = crumbs;
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
    results search(char c)
    {
        if(match(c))
        {
            if(c == 0)
            {
                return MATCHED;
            }
        }
        else
        {
            ++crumbs;
            if(std::memcmp(crumbs->name, marker->name, pos) == 0)
            {
                // If next crumb begins with same characters as last crumb,
                // then we're still in the game for searching. i.e:
                // 1. incoming key is 'hi2u'
                // 2. crumbs are: 'hi' and 'hi2u'
                // In that case marker has 'hi' in it and pos is 2

                // TODO: A limited while loop makes sense here to blast
                // through all near matches
            }
        }

        return NO_MATCH;
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