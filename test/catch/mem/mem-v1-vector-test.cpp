#include <catch2/catch_all.hpp>

#include <random>

#include <embr/mem/v1/mixins.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>
#include <embr/mem/v1/vector.h>

#include "test-mem-data.h"

using namespace embr;

template <class T, class Pool, Pool* pool = nullptr>
class vector_revealed : public mem::vector<T, Pool, pool>
{
    using base_type = mem::vector<T, Pool, pool>;

public:
    using base_type::impl;

    template <class ...Args>
    constexpr vector_revealed(Args&&...args) :
        base_type(std::forward<Args>(args)...) {}
};

// DEBT: 'battery' test ought to appear in battery-test.cpp.  That one is getting pretty big though, so putting
// vector-specific one here
// DEBT: Top-level Pool being just a generic class like this is clumsy.  I'd prefer a pool<Traits>.
// reverse-determining pool_type from pool_ops kind of a trick here, so we need Pool
template <class Pool>
static void battery(Pool& pool, int it, unsigned seed)
{
    using bytes = mem::bytes_unit<unsigned>;
    using pool_type = Pool;
    using ops_type = typename pool_type::ops_type;
    using bundle = typename ops_type::bundle;

    ops_type& ops = pool.ops();

    using namespace mem::detail::v1;
    std::mt19937 gen{seed}; // fixed seed: deterministic sequence

    CAPTURE(it, seed);

    mem::vector<short, pool_type> vector(&pool), vector2(&pool);

    auto& revealed = (vector_revealed<short, pool_type>&) vector;

    std::uniform_int_distribution<int> sz_distrib(0, 50);

    for(int i = 0; i < 10; ++i)
    {
        INFO("Phase 1");

        unsigned sz = sz_distrib(gen);

        CAPTURE(i, sz);

        vector.reserve(sz);

        assert(vector.capacity() >= sz);

        // DEBT: Roundabout (but effective) way of getting at bundle.  We need to re-acquire
        // because vector.handle_ is subject to change
        bundle bn = ops.get_bundle(revealed.impl().handle());

        bytes found_sz = ops.logical_size(bn);
        // DEBT: Be careful, not confident aliasing rules always gives us the right sizeof
        // for vector_control here
        bytes expected_sz(sizeof(mem::detail::vector_control<short>) + sz * sizeof(short));

        CAPTURE(found_sz, expected_sz);

        assert(found_sz >= expected_sz);
    }

    vector.push_back(1);
    vector2.push_back(2);

    for(int i = 0; i < 10; ++i)
    {
        unsigned sz1 = sz_distrib(gen);
        unsigned sz2 = sz_distrib(gen);

        CAPTURE(i, sz1, sz2);

        assert(vector.reserve(sz1));
        assert(vector2.reserve(sz2));

        CAPTURE(vector[0], vector2[0]);

        assert(vector[0] == 1);
        assert(vector2[0] == 2);
    }
}


TEST_CASE("gc mem v1 vector", "[memory][gc][vector]")
{
    using pool_type = mem::v1::layer1::pool<2048, 8>;
    using ops_type = pool_type::ops_type;
    using handles_type = ops_type::handles_type;
    using bundle = ops_type::bundle;
    using const_bundle = ops_type::const_bundle;
    using page_type = pool_type::page_type;
    using handle_type = pool_type::handle_type;
    using lock_handle = mem::detail::v1::lock_handle<pool_type>;
    using bytes = mem::bytes_unit<unsigned>;
    pool_type pool;

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    SECTION("vector_impl")
    {
        using type = mem::detail::vector_control<char>;
        type vi;

        REQUIRE(vi.data() == ((char*)&vi) + sizeof(type));
    }
    SECTION("vector: short")
    {
        using vector_type = mem::vector<short, pool_type>;

        constexpr unsigned control_size = sizeof(mem::detail::vector_control<short>);

        // DEBT: Aliasing rules dictate we'd prefer this to be 8 for x64 systems
        static_assert(control_size == 4);
        // DEBT: For foreseeable future x64 == 8 aliasing, but still hardcoding this isn't good
        static_assert(ops_type::aliasing == 8);

        vector_type vector(&pool);
        const auto& revealed = (vector_revealed<short, pool_type>&)vector;

        vector.reserve(27);

        const_bundle bn = revealed.impl().get_bundle();

        bytes sz = pool.ops().logical_size(bn);

        REQUIRE(sz >= control_size + 27 * 2);

        vector.reserve(12);

        sz = pool.ops().logical_size(bn);

        REQUIRE(sz >= control_size + 12 * 2);

        vector[0] = 5;
        REQUIRE(vector[0] == 5);
    }
    SECTION("vector: int")
    {
        using vector_type = mem::vector<int, pool_type>;
        const handles_type& handles = pool.ops().handles();

        handles_type::const_iterator it = handles.cbegin();

        // Always 1 'free' handle, skip him
        bool valid = ++it == handles.end();
        REQUIRE(valid);

        vector_type vector(&pool);
        const auto& revealed = (vector_revealed<int, pool_type>&)vector;

        vector.push_back(1);
        vector.push_back(2);
        REQUIRE(pool.ops_.invariant());

        // After first push, that's when we actually allocate.  Grab and make sure
        it = handles.begin();
        // NOTE: Be aware that we can't copy this guy because get_bundle secretly takes address.  This is acceptable
        // because ops() are low level in nature and even passing in a pointer doesn't clear this up much
        const page_type& page = *it;
        const_bundle bn = pool.ops().get_bundle(page);
        const_bundle bn2 = revealed.impl().get_bundle();

        // DEBT: Ultimately displace with 'revealed' approach
        REQUIRE(bn.handle == bn2.handle);
        REQUIRE(bn.allocated());
        // DEBT: Fine tune vector padding/reservation code so that this is more predictable
        REQUIRE(pool.ops().phys_size(bn).count() == 8);

        bn = pool.ops().get_bundle(*++it);
        REQUIRE(bn.allocated() == false);
        valid = ++it == handles.end();
        // DEBT: Catch2 is unhappy if we do this particular expression in a REQUIRE
        REQUIRE(valid);

        REQUIRE(vector.size() == 2);
        {
            // NOTE: accessors should not hang around in scope too long
            auto accessor = vector.at(0);
            REQUIRE(accessor == 1);
        }
        REQUIRE(*vector.lock() == 1);
        vector.unlock();

        REQUIRE(vector[0] == 1);

        // This guy will be a true 'grow' (no memory moved)
        vector.reserve(3);

        // Indeed no memory movement occurs
        REQUIRE(handles.begin()->pos() == page.pos());

        vector_type vector1(&pool);

        // He doesn't allocate from pool until we push something, so do so
        vector1.push_back(1);

        // This guy will require memory movement
        vector.reserve(20);

        bn = revealed.impl().get_bundle();

        REQUIRE(bn.pos() != page.pos());
        // We skip relinking handle, so double check we have indeed moved to a new handle#
        REQUIRE(bn2.handle != bn.handle);
        REQUIRE(bn.handle == 2);

        vector_type copied(vector);
        const auto& copied_revealed = (vector_revealed<int, pool_type>&)copied;

        const_bundle bn_copied = copied_revealed.impl().get_bundle();

        // Remember 0 was freed up when doing vector.reserve, so it is reused for the new copied
        // vector
        REQUIRE(bn_copied.handle == 0);
        //REQUIRE(copied == vector);

        vector.clear();
        vector.shrink_to_fit();
    }
    SECTION("vector: SideEffector")
    {
        int counter = 0;

        using vector_type = mem::vector<SideEffector, pool_type>;

        {
            vector_type vector(&pool);
            std::vector<SideEffector> parity;

            vector.push_back({});

            // Calls default ctor, then move ctor
            parity.push_back({});

            REQUIRE(vector[0].value().moved_to_counter == 1);
            REQUIRE(parity[0].moved_to_counter == 1);

            vector.emplace_back(&counter);

            REQUIRE(vector[1].value().counter() == 1);
            REQUIRE(vector[1]().moved_to_counter == 0);
            REQUIRE(vector[1]().moved_from_counter == 0);
        }

        REQUIRE(counter == 0);
    }
    SECTION("vector: SideEffector*")
    {
        // NOTE: Not a primary use case.  Just testing operator-> here

        int counter = 0;

        using vector_type = mem::vector<SideEffector*, pool_type>;

        {
            vector_type vector(&pool);

            SideEffector se1(&counter);

            vector.push_back(&se1);

            REQUIRE(vector[0]->counter() == 1);
            REQUIRE((*vector[0]).constructed_);
        }
    }
    SECTION("vector: lock_guard (pinned) - EXPERIMENTAL")
    {
        using vector_type = mem::vector<int, pool_type>;
        vector_type vector(&pool);
        auto& revealed = (vector_revealed<int, pool_type>&) vector;

        vector.push_back(5);
        vector.push_back(10);

        mem::v1::pinned<vector_type> pinned(revealed.impl());

        REQUIRE(pinned[0] == 5);

        const int* i = pinned.begin();

        REQUIRE(*i++ == 5);
        REQUIRE(*i == 10);

        REQUIRE(pinned.empty() == false);
    }
    SECTION("vector: pinned_iterator (EXPERIMENTAL)")
    {
        SECTION("int")
        {
            using vector_type = mem::vector<int, pool_type>;
            vector_type vector(&pool);
            auto& revealed = (vector_revealed<int, pool_type>&) vector;

            vector.push_back(5);
            vector.push_back(10);

            auto it = revealed.impl().pinned_begin();
            [[maybe_unused]]
            auto end = revealed.impl().pinned_end();

            //REQUIRE(it != end);
            REQUIRE(*it == 5);
            REQUIRE(*++it++ == 10);
        }
        SECTION("SideEffector")
        {
            int counter = 0;
            using vector_type = mem::vector<SideEffector, pool_type>;

            {
                vector_type vector(&pool);
                auto& revealed = (vector_revealed<SideEffector, pool_type>&) vector;

                vector.emplace_back(&counter);

                auto it = revealed.impl().pinned_begin();

                REQUIRE(it->counter() == 1);
            }

            REQUIRE(counter == 0);
        }
    }
    SECTION("vector: battery")
    {
        std::mt19937 rng{12345}; // NOLINT: fixed seed desired: deterministic sequence

        for(int i = 0; i < 50; ++i)
            battery(pool, i, rng());
    }
}
