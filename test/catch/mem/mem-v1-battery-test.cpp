#include <catch2/catch_all.hpp>

#include <random>
#include <unordered_map>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

using namespace embr::mem;

#define ENABLE_BATTERY 1
#define ENABLE_RANDOM_BATTERY 1

template <class Traits>
static void battery(typename detail::pool_ops<Traits>& ops, int it, unsigned seed)
{
    using namespace detail;
    std::mt19937 gen{seed}; // fixed seed: deterministic sequence

    using ops_type = typename detail::pool_ops<Traits>;
    using block = detail::v1::block;
    using handles_traits = typename ops_type::handles_traits;
    using page_type = typename handles_traits::value_type;
    using pos_type = typename page_type::unit_type;
    using bundle = typename ops_type::bundle;
    using handle_type = typename handles_traits::size_type;
    using page_type = typename handles_traits::value_type;
    using bytes = estd::units::bytes<unsigned>;
    constexpr handle_type null = handles_traits::null;
    typename ops_type::fragmentation frag;

    std::uniform_int_distribution<int> distrib(1, 10);

    //int allocs_to_do = gen() % ops.handles_.size();
    int allocs_to_do = ops.handles().size() - 1;     // 1 handle already used for big-free-block
    int frees_to_do = std::uniform_int_distribution<int>(0, allocs_to_do)(gen);

    CAPTURE(it, seed);

    std::ostringstream last;
    std::vector<handle_type> handle_cache;

    struct metadata
    {
        bytes logical_sz;
    };

    std::unordered_map<handle_type, metadata> handle_metadata;

    for(int i = 0; i < allocs_to_do; ++i)
    {
        std::ostringstream before, after;

        INFO("Phase 1");

        block::modes mode = block::Trivial;

        // DEBT: bring back 0-byte allocation requests as a bounds check.  Maybe ops itself shouldn't
        // kick back, but higher level mode definitely would need to
        pos_type phys_sz(distrib(gen) + 1);
        bytes logical_sz = bytes(phys_sz) - block::header_size(mode);

        ops.dump(before << "\n");

        const bytes available = ops.available();

        CAPTURE(before.str(), i, phys_sz.count(), available);

        bundle bn = ops.alloc(phys_sz, block::Trivial);

        if(bn.handle != null)
        {
            void* data = ops.lock(bn);
            memset(data, 'a' + bn.handle, logical_sz.count());
            ops.unlock(bn.handle);
            handle_metadata.emplace(bn.handle, metadata { logical_sz } );
        }

        //REQUIRE((int)bn.handle != null);
        // I don't want assertions number to balloon at the moment

        // Either we have a real handle or we failed because OOM
        assert(phys_sz * ops.aliasing >= available || bn.handle != null);

        assert(ops.invariant());
    }

    for(int i = 0; i < frees_to_do; ++i)
    {
        INFO("Phase 2");

        std::ostringstream before, after;

        const handle_type handle = gen() % ops.handles().size();

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

        const auto& frag0 = frag.candidates[0];

        CAPTURE(frag0.score, (int)frag0.bundle.handle, (int)frag0.move_to.handle);
        CAPTURE(frag0.overlap, frag0.adjacent);

        if(frag0.score > 0)
        {
            assert(frag0.invariant());
            ops.defrag(frag0);
            handle_cache.push_back(frag0.bundle.handle);
        }

        ops.dump(after << "\n");

        invariant_result ir = ops.invariant();
        CAPTURE(after.str(), ir);
        assert(ir);

        if(frag0.score == 0)
        {
            last << "frag0.score == 0, i=" << i << "\n";
            continue;
        }

        //last.str("");
        last << "before(" << i << "):" << before.str();
        last << "frag0.handle=" << (int)frag0.bundle.handle << ", frag0.move_to=" << (int)frag0.move_to.handle << ' ';
        last << "frag0.score=" << frag0.score << ", frag0.overlap=" << frag0.overlap << '\n';
        last << "after:" << after.str();
    }

    for(handle_type h : handle_cache)
    {
        INFO("Phase 4");
        CAPTURE((int)h, last.str());

        bundle bn = ops.get_bundle(h);
        invariant_result r = ops.invariant();
        assert(r);
        assert(bn.allocated());

        auto data = (char*)ops.lock(bn);
        char comp = 'a' + bn.handle;
        const metadata& m = handle_metadata.at(bn.handle);
        CAPTURE(m.logical_sz);
        // DEBT: See https://github.com/malachi-iot/estdlib/issues/173
        for(int i = 0; i < m.logical_sz.count(); ++i, ++data)
            assert(*data == comp);
        ops.unlock(bn.handle);

    }
}


// Neat idea, but getting back N is more ceremony than I want
template <class T, class Gen>
std::unique_ptr<T[]> make_unique_random_size(Gen& gen, int min, int max, int* N = nullptr)
{
    std::uniform_int_distribution<> distrib(min, max);
    int count = distrib(gen);

    if(N)   *N = count;

    return std::make_unique<T[]>(count);
}


template <class T, class Gen>
std::vector<T> make_vector_random_size(Gen& gen, unsigned min, unsigned max, unsigned alias = 1)
{
    std::uniform_int_distribution<unsigned> distrib(min, max);

    return std::vector<T>(distrib(gen) / alias * alias);
}


TEST_CASE("gc mem v1 battery", "[memory][gc][battery]")
{
#if ENABLE_BATTERY
    SECTION("layer1: pseudo random")
    {
        constexpr unsigned pool_sz = 512;
        using pool_type = v1::layer1::pool<pool_sz, 8>;
        using traits = pool_type::pool_traits;

        std::mt19937 rng{12345}; // fixed seed: deterministic sequence
        //std::mt19937 rng{2}; // fixed seed: deterministic sequence
        //std::mt19937 rng{4}; // fixed seed: deterministic sequence

        for(int i = 0; i < 100; ++i)
        {
            pool_type pool;
            auto& ops = pool.ops();

            // DEBT: Still having to do this
            ops.reset();
            battery(ops, i, rng());
        }
    }
    SECTION("layer3")
    {
        using pool_type = v1::layer3::pool;
        using traits = pool_type::pool_traits;
        using handles_traits = pool_type::handles_traits;
        using page_type = handles_traits::value_type;

        SECTION("pseudo-random: layer1 parity")
        {
            auto pages = std::make_unique<page_type[]>(10);
            auto raw_pool = new char[512];

            std::mt19937 rng{12345}; // NOLINT: fixed seed: deterministic sequence

            for(int i = 0; i < 50; ++i)
            {
                pool_type pool({ pages.get(), 10 }, { raw_pool, 512 });

                auto& ops = pool.ops();

                // DEBT: Still having to do this
                ops.reset();
                battery(ops, i, rng());
            }

            delete [] raw_pool;
        }
        SECTION("pseudo-random")
        {
            std::mt19937 rng{12345}; // NOLINT: fixed seed: deterministic sequence

            auto pages = make_vector_random_size<page_type>(rng, 4, 20);
            auto raw = make_vector_random_size<char>(rng, 32, 8192, 8);

            for(int i = 0; i < 50; ++i)
            {
                CAPTURE(pages.size(), raw.size());

                pool_type pool({ pages.data(), pages.size() }, { raw.data(), raw.size() });

                auto& ops = pool.ops();

                // DEBT: Still having to do this
                ops.reset();
                battery(ops, i, rng());
            }
        }
        SECTION("force feed")
        {
            // Saw this condition fail one time, but can't reproduce
            SECTION("1")
            {
                unsigned raw_pages_sz = 17;
                unsigned raw_pool_sz = 32;
                unsigned seed = 2524755352;

                auto raw_pages = new page_type[raw_pages_sz];
                auto raw_pool = new char[raw_pool_sz];

                pool_type pool({ raw_pages, raw_pages_sz }, { raw_pool, raw_pool_sz });

                CAPTURE(raw_pages_sz, raw_pool_sz);

                auto& ops = pool.ops();

                // DEBT: Still having to do this
                ops.reset();
                battery(ops, 13, seed);

                delete [] raw_pages;
                delete [] raw_pool;
            }
        }
#if ENABLE_RANDOM_BATTERY
        SECTION("random")
        {
            std::random_device r;

            std::mt19937 rng{r()};

            // NOTE: We may want to revert to all asserts in battery so that assert count doesn't fluctuate a lot
            for(int i = 0; i < 50; ++i)
            {
                auto pages = make_vector_random_size<page_type>(rng, 4, 24);
                auto raw = make_vector_random_size<char>(rng, 32, 8192, 8);

                CAPTURE(pages.size(), raw.size());

                pool_type pool({ pages.data(), pages.size() }, { raw.data(), raw.size() });

                auto& ops = pool.ops();

                // DEBT: Still having to do this
                ops.reset();
                battery(ops, i, rng());
            }
        }
#endif
    }
#endif
}
