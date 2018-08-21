#pragma once

#include "netbuf.h"

#include <estd/forward_list.h>

namespace embr { namespace mem {

namespace experimental {

struct NetBufDynamicDefaultPolicy
{
    CONSTEXPR int size_to_allocate() const { return 128; }
};

struct NetBufDynamicChunk : estd::experimental::forward_node_base_base<NetBufDynamicChunk*>
{
    typedef estd::experimental::forward_node_base_base<NetBufDynamicChunk*> base_type;
    typedef int size_type;

    size_type size;
    uint8_t data[];

    NetBufDynamicChunk(size_type size) :
        base_type(NULLPTR),
        size(size) {}
};

// NOTE: Close, but not perfectly well suited, to our locking allocator scheme
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

    typedef NetBufDynamicChunk Chunk;

#ifdef UNIT_TESTING
public:
#endif
    estd::intrusive_forward_list<Chunk> chunks;

    typedef typename estd::intrusive_forward_list<Chunk>::iterator iterator;

    Chunk* current;

    TAllocator get_allocator() { return TAllocator(); }

    bool empty() const { return current == NULLPTR; }

    Chunk* allocate(size_type sz)
    {
        TAllocator a = get_allocator();

        Chunk* chunk = (Chunk*) allocator_traits::allocate(a, sizeof(Chunk) + sz);

        if(chunk == NULLPTR) return NULLPTR;

        new (chunk) Chunk(sz);

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
            Chunk& chunk = *it;

            ++it;

            chunk.~Chunk();

            allocator_traits::deallocate(a,
                                         (uint8_t*)&chunk,
                                         chunk.size +
                                         sizeof(Chunk));
        }
    }

    uint8_t* data() const
    {
        if(empty()) return NULLPTR;
        /*
        {
            current = allocate(size_to_allocate());
            chunks.push_front(*current);
        } */

        return current->data;
    }

    size_type size() const
    {
        if(empty()) return 0;


        return current->size;
    }

    size_type total_size() const
    {
        size_type total = 0;

        // FIX: Can't do const yet because iterator dereference doesn't
        // have a const version.  When we rememedy this, see about ->
        // operator too
        iterator it = chunks.begin();

        while(it != chunks.end())
        {
            total += (*it).size;
            it++;
        }

        return total;
    }

    bool next()
    {
        if(empty())
        {
            if(!chunks.empty())
            {
                current = &chunks.front();
                return true;
            }
        }
        else if(current->next())
        {
            current = current->next();
            return true;
        }

        return false;
    }

    // expand by allocating a brand new chunk of memory
    // auto-next will move our current pointer forward to the newly allocated chunk
    ExpandResult expand(size_type expand_by, bool auto_next)
    {
        // attempt this too, since contiguous is (often) preferred
        //realloc()

        Chunk* allocated = allocate(expand_by);

        if(allocated == NULLPTR) return ExpandResult::ExpandFailOutOfMemory;

        // if we have no chunks at this time
        if(empty())
        {
            // FIX: Beware, current == NULLPTR but chunks having
            // a value is theoretically valid (think before_begin() )
            // but is not fully thought out here so may behave in an undefined way
            // *except* for next() which has been specially treated to handle
            // this scenario
            if(auto_next) current = allocated;

            chunks.push_front(*allocated);
        }
        else
        {
            // TODO:
            // assert that current != null

            // presumes we want to tack on past where we currently are (tack
            // on to the end)
            current->next(allocated);

            if(auto_next)
                //current = current->next();
                current = allocated;
        }

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
