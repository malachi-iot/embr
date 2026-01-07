#include <catch2/catch_all.hpp>

#include <random>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"


using namespace embr::mem;

template <class Traits, class HandlesTraits>
static void battery(typename detail::pool<Traits>::template ops<HandlesTraits>& ops, int it, unsigned seed)
{
    using namespace detail;
    std::mt19937 gen{seed}; // fixed seed: deterministic sequence

    using pos_type = typename Traits::pos_type;
    using bundle = detail::bundle;
    using handle_type = typename HandlesTraits::size_type;
    using page_type = typename Traits::page_type;
    constexpr handle_type null = HandlesTraits::null;
    fragmentation frag;

    std::uniform_int_distribution distrib(1, 10);

    //int allocs_to_do = gen() % ops.handles_.size();
    int allocs_to_do = ops.handles_.size() - 1;     // 1 handle already used for big-free-block
    int frees_to_do = std::uniform_int_distribution(0, allocs_to_do)(gen);

    CAPTURE(it, seed);

    std::ostringstream last;

    for(int i = 0; i < allocs_to_do; ++i)
    {
        std::ostringstream before, after;

        INFO("Phase 1");

        // DEBT: bring back 0-byte allocation requests as a bounds check.  Maybe ops itself shouldn't
        // kick back, but higher level mode definitely would need to
        pos_type phys_sz(distrib(gen) + 1);

        ops.dump(before << "\n");

        const unsigned available = ops.available();

        CAPTURE(before.str(), i, phys_sz.count(), available);

        bundle bn = ops.alloc(phys_sz, block::Trivial);

        //REQUIRE((int)bn.handle != null);
        // I don't want assertions number to balloon at the moment

        // Either we have a real handle or we failed because OOM
        assert(phys_sz.count() * ops.aliasing >= available || bn.handle != null);

        assert(ops.invariant());
    }

    for(int i = 0; i < frees_to_do; ++i)
    {
        INFO("Phase 2");

        std::ostringstream before, after;

        const handle_type handle = gen() % ops.handles_.size();

        ops.dump(before << "\n");

        CAPTURE(before.str(), i, handle);

        bundle bn = ops.get_bundle(handle);

        // FIX: Having a lot of issues here.  Granted, bn isn't to be used on null
        // handles and null pages, but it should work.  And also, dealloc itself dies
        if(bn.page->is_null() == false && bn.is_null() == false && bn.allocated())
        {
            ops.dealloc(bn);
            bn = ops.get_bundle(handle);
            assert(bn.invariant());
        }

        ops.dump(after << "\n");

        CAPTURE(after.str());

        invariant_result r = ops.invariant();
        if(!r)
        {
            const invariant_violation& err = r.error();
            CAPTURE(err.rule, err.details);
            assert(false);
        }
    }

    for(int i = 0; i < frees_to_do; ++i)
    {
        INFO("Phase 3");

        std::ostringstream before, after;

        ops.dump(before << "\n");

        CAPTURE(last.str());
        CAPTURE(before.str(), i);

        ops.assess(&frag);

        auto& frag0 = frag.candidates[0];

        CAPTURE(frag0.score, frag0.bundle.handle, frag0.move_to.handle);

        if(frag0.score > 0)
        {
            assert(frag0.invariant());
            //ops.defrag(frag0);
        }

        ops.dump(after << "\n");

        CAPTURE(after.str());
        REQUIRE(ops.invariant());

        //last.str("");
        last << "before(" << i << "):" << before.str();
        last << "frag0.handle=" << (int)frag0.bundle.handle << ", frag0.move_to=" << (int)frag0.move_to.handle << "\n";
        last << "after:" << after.str();
    }
}


TEST_CASE("gc mem v1 battery", "[memory][gc][battery]")
{
    SECTION("layer1")
    {
        constexpr unsigned pool_sz = 512;
        using pool_type = v1::layer1::pool<pool_sz, 8>;
        using traits = pool_type::pool_traits;

        std::mt19937 rng{12345}; // fixed seed: deterministic sequence
        //std::mt19937 rng{2}; // fixed seed: deterministic sequence
        //std::mt19937 rng{4}; // fixed seed: deterministic sequence

        // FIX: Next up is defrag move doesn't fully update next handle, creating a circular list
        for(int i = 0; i < 100; ++i)
        {
            pool_type pool;
            auto ops = pool.ops();

            // DEBT: Still having to do this
            ops.reset();
            battery<traits>(ops, i, rng());
        }
    }
}
