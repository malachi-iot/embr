#pragma once

#include "netbuf.h"

#include <estd/forward_list.h>

namespace embr { namespace mem {

namespace experimental {

struct NetBufDynamicDefaultPolicy
{
    CONSTEXPR int size_to_allocate() const { return 128; }
};

template <class TAllocator = std::allocator<uint8_t> >
class NetBufDynamic
{
public:
    typedef int size_type;
    typedef TAllocator allocator_type;
    typedef std::allocator_traits<TAllocator> allocator_traits;
    typedef NetBufDynamicDefaultPolicy policy_type;

private:
    policy_type get_policy() const { return policy_type(); }

    CONSTEXPR size_type size_to_allocate() const
    { return get_policy().size_to_allocate(); }

    struct Chunk : estd::experimental::forward_node_base_base<Chunk*>
    {
        size_type size;
        uint8_t data[];
    };

    estd::intrustive_forward_list<Chunk> chunks;

    typedef typename estd::intrustive_forward_list<Chunk>::iterator iterator;

    Chunk* current;

    TAllocator get_allocator() { return TAllocator(); }

    bool empty() { return current != NULLPTR; }

    Chunk* allocate(size_type sz)
    {
        TAllocator a = get_allocator();

        Chunk* chunk = (Chunk*) allocator_traits::allocate(a, sizeof(Chunk) + sz);

        chunk->size = sz;

        return chunk;
    }

public:
    NetBufDynamic() : current(NULLPTR) {}

    ~NetBufDynamic()
    {
        iterator it = chunks.begin();
        TAllocator a = get_allocator();

        while(it != chunks.end())
        {
            Chunk* chunk = &(*it);
            iterator it_next = it+1;

            allocator_traits::deallocate(a,
                                         (uint8_t*)chunk,
                                         chunk->size +
                                         sizeof(Chunk));

            it = it_next;
        }
    }

    uint8_t* data()
    {
        if(empty())
        {
            current = allocate(size_to_allocate());
            chunks.push_front(*current);
        }

        return current->data;
    }

    size_type size()
    {
        if(empty())
        {
            current = allocate(size_to_allocate());
            chunks.push_front(*current);
        }

        return 128;
    }

    bool next()
    {
        if(!empty() && current->next())
        {
            current = current->next();
            return true;
        }

        return false;
    }

    ExpandResult expand(size_type expand_by, bool auto_next)
    {
        // attempt this too, since contiguous is (often) preferred
        //realloc()

        // presumes we want to tack on past where we currently are
        current->next(allocate(expand_by));

        if(auto_next) current = current->next();

        return ExpandResult::ExpandOKChained;
    }

    bool last()
    {
        if(!empty()) { return current->next() == NULLPTR; }
        return true;
    }


    void reset()
    {
        current = &chunks.front();
    }
};

}

}}
