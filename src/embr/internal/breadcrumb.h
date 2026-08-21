#pragma once

#include <estd/string.h>

#include "breadcrumb/searcher.h"
#include "breadcrumb/traits.h"

// Reinterpretation of embr::coap 'triple' / URI mapper

namespace embr { namespace internal {

inline const breadcrumb* search2(const breadcrumb* crumbs, const char* s)
{
    using traits = breadcrumb_traits<breadcrumb>;
    using searcher = breadcrumb_searcher<traits>;
    searcher srch{crumbs};
    breadcrumb_functor<traits> functor{&srch};

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

constexpr bool has_children(const breadcrumb* crumbs)
{
    using traits = breadcrumb_traits<breadcrumb>;

    // Paradigm is such that if children exist, they are the very next item after
    // the parent
    return traits::is_child(*crumbs, *(crumbs + 1));
}

/// Return the first child of a given parent node
/// @param parent
/// @return nullptr if no children or end marker, otherwise pointer to first child
inline ESTD_CPP_CONSTEXPR(14) const breadcrumb* first_child(const breadcrumb* parent)
{
    using traits = breadcrumb_traits<breadcrumb>;

    assert(parent);
    assert(!traits::is_null(*parent));

    const breadcrumb* child = parent + 1;

    return traits::is_child(*parent, *child) ? child : nullptr;
}

inline ESTD_CPP_CONSTEXPR(14) const breadcrumb* next_sibling(const breadcrumb* crumbs)
{
    using traits = breadcrumb_traits<breadcrumb>;

    assert(crumbs);
    assert(!traits::is_null(*crumbs));

    const breadcrumb* sibling = crumbs + 1;

    if(traits::is_null(*sibling)) return nullptr;

    return traits::is_sibling(*sibling, *crumbs) ? sibling : nullptr;
}

/// Search siblings, inclusive
/// @param crumbs
/// @param name
/// @return
/// DEBT: Too permissive, ADL is gonna go crazy here on breadcrumb match
template <class Breadcrumb = breadcrumb, class Impl>
ESTD_CPP_CONSTEXPR(14) const Breadcrumb* search_siblings(const Breadcrumb* crumbs,
    const estd::detail::basic_string<Impl>& name)
{
    const int parent = crumbs->parent;
    using traits = breadcrumb_traits<Breadcrumb>;
    for(;!traits::is_null(*crumbs); ++crumbs)
    {
        // DEBT: We could stop searching if we leave siblings area too.  I think we can look for a parent id
        // smaller than our own.  Not 100% sure yet though.  Alternatively, track that they're all true children
        // (requiring an id stack or recursion).  In the meantime, we're just doing some extra/unnecessary searching
        // but no risk of a false positive
        if(crumbs->parent != parent) continue;  // Skip children in hopes we find another sibling

        if(name == crumbs->name) return crumbs;
    }

    // No match
    return nullptr;
}

inline const breadcrumb* search_siblings(const breadcrumb* crumbs,
    const char* name)
{
    return search_siblings(crumbs, estd::layer2::const_string(name));
}


}}

namespace embr { namespace breadcrumb { inline namespace v1 {

template <class T>
using traits = embr::internal::breadcrumb_traits<T>;
using breadcrumb = embr::internal::breadcrumb;
using functor = embr::internal::breadcrumb_functor<traits<breadcrumb>>;
using searcher = embr::internal::breadcrumb_searcher<traits<breadcrumb>>;

}}}
