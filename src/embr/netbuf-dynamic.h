/**
 *  @file
 */
#pragma once

#include "netbuf.h"

#include <estd/forward_list.h>

namespace embr { namespace mem {

namespace experimental {

struct NetBufDynamicDefaultPolicy
{
    // represents minimum size to allocate
    CONSTEXPR int minimum_allocation_size() const { return 128; }
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
template <class TAllocator = std::allocator<uint8_t>,
          class TPolicy = NetBufDynamicDefaultPolicy >
class NetBufDynamic
{
public:
    typedef int size_type;
    typedef TAllocator allocator_type;
    typedef std::allocator_traits<TAllocator> allocator_traits;
    typedef TPolicy policy_type;

private:
    policy_type get_policy() const { return policy_type(); }

    CONSTEXPR size_type minimum_allocation_size() const
    { return get_policy().minimum_allocation_size(); }

    typedef NetBufDynamicChunk Chunk;

#ifdef UNIT_TESTING
public:
#endif
    estd::intrusive_forward_list<Chunk> chunks;

    typedef typename estd::intrusive_forward_list<Chunk>::iterator iterator;

    Chunk* current;

    TAllocator get_allocator() { return TAllocator(); }

    bool empty() const { return current == NULLPTR; }

    Chunk* allocate(size_type sz, bool override_minimum = false)
    {
        TAllocator a = get_allocator();

        if(sz < minimum_allocation_size() && !override_minimum)
            sz = minimum_allocation_size();

        Chunk* chunk = (Chunk*) allocator_traits::allocate(a, sizeof(Chunk) + sz);

        if(chunk == NULLPTR) return NULLPTR;

        new (chunk) Chunk(sz);

        return chunk;
    }

    static void deallocate(TAllocator& a, Chunk& chunk, bool run_destructor = true)
    {
        if(run_destructor)
            chunk.~Chunk();

        allocator_traits::deallocate(a,
                                     (uint8_t*)&chunk,
                                     chunk.size +
                                     sizeof(Chunk));
    }

public:
    NetBufDynamic() : current(NULLPTR) {}

    ~NetBufDynamic()
    {
        iterator it = chunks.begin();
        // FIX: going to need to do the ref/non ref dance here
        // for stateful allocators
        allocator_type a = get_allocator();

        while(it != chunks.end())
        {
            // have to use temporary because otherwise we are using
            // a deleted iterator
            iterator del_it = it++;

            deallocate(a, *del_it);
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


    ///
    /// \brief shrinks last (current) chained buffer to specified size
    /// \param to_size size to shrink to, or 0 to deallocate
    ///
    void shrink_experimental(size_type to_size)
    {
        if(empty()) return;

        iterator it = chunks.begin();

        size_type tally = (*it).size;

        // FIX: going to need to do the ref/non ref dance here
        // for stateful allocators
        allocator_type a = get_allocator();

        // TODO: We need a 'before begin' for this to work right
        /*
        if((*it).next() == NULLPTR)
        {
            // fake before-begin version
        } */

        // only enter this loop while we have entire chunks to save
        while(to_size > tally)
        {
            // since to_size is still larger than our current tally, we
            // move forward through chunks (undefined behavior if to_size
            // > total_size())
            Chunk& c = *++it;

            tally += c.size;
        }

        // we should only arrive here while still having a valid it, meaning we can it++
        // safely.  we reassign current because shrinking means we are at the last
        // valid chunk post-shrinkage
        current = &(*it);

        if(to_size == tally)
        {
            // don't need to shrink, exact match after
        }
        else
        {
            // FIX: this is not going to place nice with all deallocators, because
            // it will be passing wrong size into deallocation.  What
            // we really need to do is a realloc here, but that gets complicated
            // because we have to potentially reinsert the item into the linked
            // list (node + data allocated together and a realloc could change pointer
            // location)
            current->size -= tally - to_size;
        }

        iterator del_after = it++;

        // now that we've skipped allocated chunks who fit, loop
        // over ones who are gonna be deleted because of the shrink
        while(it != chunks.end())
        {
            // NOTE: Should we expect destructor to run here? Hmm, that would be bad
            // http://www.cplusplus.com/reference/forward_list/forward_list/erase_after/ indicates yes
            chunks.erase_after(del_after++);

            iterator to_del = it++;

            deallocate(a, *to_del);
        }
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
