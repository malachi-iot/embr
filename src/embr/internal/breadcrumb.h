#pragma once

#include <estd/string.h>
#include <estd/vector.h>

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

template <class Breadcrumb>
constexpr bool has_children(const Breadcrumb* crumbs)
{
    using traits = breadcrumb_traits<Breadcrumb>;

    // Paradigm is such that if children exist, they are the very next item after
    // the parent
    return traits::is_child(*crumbs, *(crumbs + 1));
}

// DEBT: estd::optional can't be treated like a literal here, make an issue for that
template <class Node, class F, class Traits = breadcrumb_traits<Node>>
ESTD_CPP_CONSTEXPR(14) const Node* visit_children(const Node* node, F&& f,
    bool sibling_mode = false)
{
    using traits = Traits;
    using int_type = typename traits::int_type;

    int_type parent = traits::id(*node);
    estd::layer1::vector<int_type, 8> nav;

    auto is_null = [](const Node* node) { return traits::is_null(*node); };
    auto is_child = [](int_type parent, const Node* child)
    {
        // DEBT: Do a traits for this type of comparison also
        return parent == child->parent;
        //return traits::is_child(*parent, *child);
    };

    for(;!is_null(node);)
    {
        if(sibling_mode)
        {
            // sibling mode means that first node is actually a sibling, so
            // set up parent based on passed in sibling_parent and don't
            // initially increment node
            parent = node->parent;
            nav.push_back(parent);
            sibling_mode = false; // After initial setup, everything otherwise operates without edge case processing
        }
        else
            // Navigate to first child, if it exists
            ++node;

        // iterate through first-level children (siblings)
        for(;is_child(parent, node); ++node)
        {
            if(is_null(node))   return nullptr;

#if __cpp_if_constexpr
            using return_type = decltype(f(node));

            if constexpr (estd::is_void<return_type>::value)
            {
                f(node);
            }
            else
            {
                std::optional<Node*> r = f(node);

                if(r.has_value())   return *r;
            }
#else
            f(node);
#endif
        }

        // if it's not a first-level child, then we either went up or down the hierarchy.
        // Have a look
        if(traits::is_child(*(node - 1), *node))
        {
            // we've gone down (deeper) into hierarchy

            parent = traits::id(*--node);

            assert(nav.size() < nav.max_size());

            nav.push_back(parent);
        }
        else
        {
            // if not down, then up.  How far up?  See if we can find the owning parent
            // of the presented node
            for(; !nav.empty() &&
                !is_child(nav.back(), node); nav.pop_back())   {}

            // We only track ourself and children.  If we can't find a relationship
            // in that list, that's enough and we're done.
            if(nav.empty()) return node;

            parent = nav.back();

            --node;
        }
    }
    return nullptr;
}

template <class Node>
constexpr const Node* skip_children(const Node* parent)
{
    return visit_children(parent, [](auto){});
}


/// Return the first child of a given parent node
/// @param parent
/// @return nullptr if no children or end marker, otherwise pointer to first child
template <class Breadcrumb>
ESTD_CPP_CONSTEXPR(14) const Breadcrumb* first_child(const Breadcrumb* parent)
{
    using traits = breadcrumb_traits<Breadcrumb>;

    assert(parent);
    assert(!traits::is_null(*parent));

    const Breadcrumb* child = parent + 1;

    return traits::is_child(*parent, *child) ? child : nullptr;
}

template <class Breadcrumb>
ESTD_CPP_CONSTEXPR(14) const Breadcrumb* next_sibling(const Breadcrumb* crumbs)
{
    using traits = breadcrumb_traits<Breadcrumb>;

    assert(crumbs);
    assert(!traits::is_null(*crumbs));

    const Breadcrumb* sibling = crumbs + 1;

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
    const estd::detail::basic_string<Impl>& name, bool sorted = false)
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

        // traits::name might return const char* or a string_view, so start compare with
        // passed in name who always has a compare.  Perhaps https://github.com/malachi-iot/estdlib/issues/232
        // can offer some oblique assistance
        const int r = name.compare(traits::name(*crumbs));

        if(r == 0)
            return crumbs;
        // r < 0 means that our name lexigraphically sorts before name in breadcrumbs.  We want to always
        // appear before or on sorted names.  For example:
        // d >  a - yes, keep searching
        // d >  b - yes, keep searching
        // d <  e - no, no further searching needed
        else if(sorted && r < 0)
            return nullptr;
    }

    // No match
    return nullptr;
}


// NOT READY YET
template <class Node = breadcrumb, class Impl>
ESTD_CPP_CONSTEXPR(14) const Node* search_siblings_new(const Node* top,
    const estd::detail::basic_string<Impl>& name, bool sorted = false)
{
    using traits = breadcrumb_traits<Node>;
    const typename traits::int_type parent = top->parent;

    return visit_children(top, [&](const Node* node) -> std::optional<Node*>
        {
            if(node->parent != parent) return {};  // Skip children in hopes we find another sibling

            // traits::name might return const char* or a string_view, so start compare with
            // passed in name who always has a compare.  Perhaps https://github.com/malachi-iot/estdlib/issues/232
            // can offer some oblique assistance
            const int r = name.compare(traits::name(*node));

            if(r == 0)
                return node;
            // r < 0 means that our name lexigraphically sorts before name in breadcrumbs.  We want to always
            // appear before or on sorted names.  For example:
            // d >  a - yes, keep searching
            // d >  b - yes, keep searching
            // d <  e - no, no further searching needed
            else if(sorted && r < 0)
                return nullptr;

            return {};

        }, true);
}

template <class Breadcrumb>
constexpr const Breadcrumb* search_siblings(const Breadcrumb* crumbs,
    const char* name, bool sorted = false)
{
    return search_siblings(crumbs, estd::layer2::const_string(name), sorted);
}


/// Investigate 'current' to see if its children match 'v'
/// @param v
/// @param top top-level node nav tree
/// @param current nullptr (for virtual root node) otherwise node whose children to search
/// @return
/// @remarks Remember, breadcrumbs specifically do not search grandchildren too.  It's one generation at a time.
template <class Breadcrumb, class String>
ESTD_CPP_CONSTEXPR(17) static const Breadcrumb* search_children(
    const String& v, const Breadcrumb* top, const Breadcrumb* current)
{
    // When just starting, pretend to have root node so search siblings without a child
    current = current == nullptr ? top : first_child(current);
    return search_siblings(current, v);
}

}}

namespace embr { namespace breadcrumb { inline namespace v1 {

template <class T>
using traits = embr::internal::breadcrumb_traits<T>;
using breadcrumb = embr::internal::breadcrumb;
using functor = embr::internal::breadcrumb_functor<traits<breadcrumb>>;
using searcher = embr::internal::breadcrumb_searcher<traits<breadcrumb>>;

}}}
