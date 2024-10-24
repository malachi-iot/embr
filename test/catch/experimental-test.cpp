#include <catch2/catch_all.hpp>

#include <memory>

#include <estd/string.h>
#include <estd/string_view.h>

// NOTE: At this time, not yet freertos specific, so test it here in regular GNU area
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/streambuf.hpp>
#include <estd/sstream.h>

#include <embr/bits/bits.hpp>

#include <embr/platform/esp-idf/rebase.h>

#include "test-data.h"
#include "rebase-test.h"

using namespace embr::experimental;

template <class TAllocator>
class test_string : public estd::basic_string<
        char,
        std::char_traits<char>,
        TAllocator>
{
    typedef estd::basic_string<
            char,
            std::char_traits<char>,
            TAllocator> base;

public:
    test_string(TAllocator& a) : base(a) {}

    // NOTE: makes no difference
    typedef typename std::remove_reference<TAllocator>::type allocator_type;
};


template <class TAllocator>
void test_alloc(TAllocator& a)
{
    // FIX: Misbehaves when we really want it to be TNetBufAllocator&
    // runs destructor since we're actually copying to a local TNetBufAllocator
    // rather than a reference, which results in two active TNetBufAllocators
    // - at the moment it doesn't crash, but it will
    test_string<TAllocator&> s(a);

    s += "hello";

    // FIX: Although embr netbufs are phased out, this clang incompatibility
    // belies a deeper estd allocator problem
#ifndef __clang__
    REQUIRE(s == "hello");

    s += " world!";

    REQUIRE(s == "hello world!");
#endif
}


// NOTE: scheduler defaults to a flavor of 'greater'
// std::priority_queue 'less' results in a max queue.  I like to think of this
// as priority_queue sorts things backwards.  So, returning 'true' from your
// comparison means the 'left' will tumble to the end.
template <class T, class TCompare = estd::less<typename T::key_type> >
struct BatchCompareTraits;

template <class T, class TCompare>
struct BatchCompareTraits
{
    ESTD_CPP_STD_VALUE_TYPE(T)

    typedef typename value_type::key_type key_type;
    typedef TCompare key_compare;

    static constexpr int batch_id(const_reference v) { return v.batch_id(); }
    static constexpr const key_type& key(const_reference v) { return v.key(); }
};

static int breakpointCondition = 0;

template <class T, class TTraits = BatchCompareTraits<T> >
struct BatchCompare : TTraits::key_compare
{
    ESTD_CPP_STD_VALUE_TYPE(T)

    typedef TTraits traits_type;
    typedef typename traits_type::key_compare key_compare;
    typedef typename traits_type::key_type key_type;

    /*
    int* current_batch_;

    void flip() { *current_batch_ = !*current_batch_; }

    int current_batch() const { return *current_batch_; }

    BatchCompare(int* current_batch) : current_batch_(current_batch) {} */
    int current_batch_ = 0;
    void flip() { current_batch_ = !current_batch_; }
    int current_batch() const { return current_batch_; }

    bool operator ()(const_reference left, const_reference right) const
    {
        // TODO: We'll need to return to this for scheduler
        //time_point l_tp = impl_type::get_time_point(left);
        //time_point r_tp = impl_type::get_time_point(right);
        //return l_tp > r_tp;

        bool is_left_current_batch = current_batch() == traits_type::batch_id(left);
        bool is_right_current_batch = current_batch() == traits_type::batch_id(right);

        // left is current, right is not, therefore right comes first (older batch comes first)
        if(is_left_current_batch && !is_right_current_batch) return true;

        // right is current, left is not, therefore left comes first (older batch comes first)
        if(!is_left_current_batch && is_right_current_batch) return false;

        // both right and left are in the same batch, so regular key compare ensues
        return key_compare::operator()(traits_type::key(left), traits_type::key(right));
    }
};

struct BatchKey
{
    typedef int key_type;

    const int batch_id_;
    const int key_;

    constexpr int batch_id() const { return batch_id_; }
    constexpr const key_type& key() const { return key_; }

    // DEBT: single_allocator_base needs this and I keep forgetting if that's reasonable
    // (i.e. std::vector also demands a default constructor.... right?)
    BatchKey() : batch_id_(0), key_(0) {}

    constexpr BatchKey(int batch_id, int key) :
        batch_id_(batch_id),
        key_(key)
    {}
    BatchKey(const BatchKey& copy_from) = default;

    // DEBT: Priority queue demands this - perhaps reasonably so
    BatchKey& operator=(const BatchKey& copy_from)
    {
        return * new (this) BatchKey(copy_from);
    }

    constexpr bool operator==(const BatchKey& compare_to) const
    {
        return batch_id_ == compare_to.batch_id_ &&
            key_ == compare_to.key_;
    }
};

std::ostream& operator << ( std::ostream& os, const BatchKey& value )
{
    os << value.batch_id() << ':' << value.key();
    return os;
}


template <class T, class Container, class TTraits = BatchCompareTraits<T> >
struct BatchHelper
{
    typedef BatchCompare<T, TTraits> comp_type;
    typedef estd::priority_queue<T, Container, comp_type > queue_type;
    typedef typename queue_type::accessor accessor;
    typedef typename comp_type::key_type key_type;
    typedef typename comp_type::value_type value_type;

    comp_type& comp;
    queue_type& queue;

    key_type current;
    static constexpr key_type max() { return 20; }

    BatchHelper(queue_type& queue) :
        comp(queue.compare()),
        queue(queue)
    {}

    // Undefined if to_add itself is greater than max
    static bool would_add_rollover(key_type v, key_type to_add, key_type max)
    {
        // v + to_add > max
        // v > max - to_add
        return v > (max - to_add);
    }

    template <class ...TArgs>
    accessor emplace(key_type delta, TArgs&&...args)
    {
        if(would_add_rollover(current, delta, max()))
        {
            // DEBT: This technically doesn't avoid a rollover, only used for synthetic testing
            // DEBT: Even if it did handle rollover, we want to selectively use explicit rollover code
            // based on numeric_limits::max
            current = current + delta - max();

            comp.flip();
        }
        else
            current += delta;

        return queue.emplace(comp.current_batch(), current, std::forward<TArgs>(args)...);
    }

    // Helper just for unit testing
    void pop_and_compare(value_type compare_to)
    {
        value_type v = queue.top();
        queue.pop();

        REQUIRE(v == compare_to);
    }
};

template <class T, class Container, class TTraits = BatchCompareTraits<T> >
BatchHelper<T, Container, TTraits> make_batch_helper(
    estd::priority_queue<T, Container, BatchCompare<T, TTraits> >& q)
{
    return BatchHelper<T, Container, TTraits>(q);
}

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("Retry v3")
    {
        // stringbufs still a mess, so using span variety
        //typedef estd::experimental::ostringstream<128> ostream_type;
        typedef estd::detail::ospanstream ostream_type;
        typedef estd::detail::ispanstream istream_type;

        struct Transport
        {
            typedef int endpoint_type;

            //typedef estd::layer1::stringbuf<128> ostreambuf_type;
            typedef ostream_type::streambuf_type ostreambuf_type;
            typedef istream_type::streambuf_type istreambuf_type;
        };

        struct RetryImpl
        {
            typedef Transport transport_type;
            typedef int key_type;
            typedef unsigned timebase_type;

            struct item_policy_impl_type
            {
                timebase_type get_new_expiry()
                {
                    return 100;
                }
            };

            timebase_type get_relative_expiry(item_policy_impl_type& item)
            {
                return 100;
            }
        };


        struct TimerImpl
        {
            typedef unsigned timebase_type;
            typedef int handle_type;

            handle_type create(timebase_type expiry, void* arg)
            {
                return 0;
            }
        };

        char buf[128];
        estd::span<char> span(buf);

        ostream_type out(span);

        int fake_endpoint = 7;
        auto sb = out.rdbuf();

        embr::experimental::RetryManager<Transport, RetryImpl, TimerImpl> rm;

        // FIX: In its current state, this generates a memory leak since send does a 'new'
        rm.send(fake_endpoint, *sb, 0);
    }
    SECTION("bits")
    {
        using namespace embr;

        SECTION("optimized/low level getter ('v3')")
        {
            // ambiguous
            //typedef
                //bits::internal::getter<embr::word<10>, bits::little_endian, bits::lsb_to_msb > getter;
            //typedef
                //bits::internal::getter<bits::little_endian, bits::lsb_to_msb > getter;
            typedef
                bits::detail::getter<4, 8, bits::little_endian, bits::lsb_to_msb> getter;

            //getter::get2<0, 4>(le_example1);
            //auto v = getter::get<4, 8, uint16_t>(le_example1 + 3);
            uint16_t v;
            getter::get(le_example1 + 3, v);

            REQUIRE(v == 0x23);

            SECTION("wrapped in decoder")
            {
                // DEBT: Get a more useful constructor here to copy in external data if so desired
                // DEBT: Use layer2 here
                bits::layer1::decoder<bits::little_endian, 8> d;

                estd::copy_n(le_example1, 4, d.data());

                // grabs middle of LE 32-bit integer, 4 bits in and treated
                // as an 8-bit value
                auto v = d.get<4, 8>(2);

                REQUIRE(v.cvalue() == 0x23);

                auto v2 = d.get<4, 12>(2);

                // This is accurate.  Remember, bitpos is trims off the left END of the word
                // for little endian, thus pushing length further towards the right
                REQUIRE(v2 == 0x123);

                auto v3 = d.get<4, 16>(1);

                REQUIRE(v3.cvalue() == 0x2345);
            }
        }
        SECTION("optimized setter")
        {
            uint8_t raw[8];

            estd::fill_n(raw, 8, 0);

            // DEBT: These 3 no longer experimental, move them
            REQUIRE(embr::bits::internal::is_subbyte(0, 8) == true);
            REQUIRE(embr::bits::internal::is_byte_boundary(0, 8) == true);
            REQUIRE(embr::bits::internal::is_valid(0, 8) == true);

            SECTION("little endian")
            {
                bits::experimental::set<4, 8, bits::little_endian, bits::lsb_to_msb>(raw, 0x23);

                REQUIRE((int)raw[0] == 0x30);       // lsb_to_msb bitpos=4 means we start 4 bits in
                REQUIRE((int)raw[1] == 0x02);       // lsb_to_msb resume means we start from lsb 0
            }
            SECTION("big endian")
            {
                typedef
                    bits::setter<bits::big_endian, bits::lsb_to_msb > setter;

                SECTION("within-byte operations")
                {
                    setter::set<4, 3>(raw + 1, 2);

                    REQUIRE((int)raw[1] == 0x20);

                    setter::set(bits::descriptor{0, 3}, raw + 1, 2);

                    REQUIRE((int)raw[1] == 0x22);
                }
                SECTION("multi byte operations")
                {
                    //setter::set<4, 8>(raw, 0x23); // disabled as we haven't implemented this specialization yet
                    setter::set<0, 16>(raw, 0x23);
                    setter::set(bits::descriptor{0, 32}, raw + 2, 0x1234);

                    REQUIRE((int)raw[0] == 0);
                    REQUIRE((int)raw[1] == 0x23);

                    REQUIRE((int)raw[4] == 0x12);
                    REQUIRE((int)raw[5] == 0x34);
                }
            }
        }
    }
    SECTION("batched priority queue")
    {
        //int current_batch = 0;
        //BatchCompare<BatchKey> c(&current_batch);
        //estd::layer1::priority_queue<BatchKey, 10, BatchCompare<BatchKey> > q(c);
        estd::layer1::priority_queue<BatchKey, 10, BatchCompare<BatchKey> > q;
        BatchCompare<BatchKey>& c = q.compare();    // DEBT: Not sure I like naming the accessor 'compare'

        auto helper = make_batch_helper(q);

        constexpr BatchKey k1(0, 5), k2(0, 10), k3(1, 3), k4(1, 7), k5(1, 11);
        constexpr int max = 10;

        // Works also, I just like the succinctness of k1, k2, etc
        //q.emplace(c.current_batch(), 5);
        //q.emplace(c.current_batch(), 10);

        q.push(k1);
        q.push(k2);

        BatchKey k;

        SECTION("normal operations")
        {
            k = q.top();

            REQUIRE(k == k2);

            q.pop();

            k = q.top();

            REQUIRE(k == k1);
        }
        SECTION("batched operation")
        {
            c.flip();           // 1:xxx elements are now current ones

            REQUIRE(c.current_batch() == 1);

            breakpointCondition = 1;
            q.push(k3);         // 1:3
            breakpointCondition = 0;

            k = q.top();
            q.pop();

            REQUIRE(k == k2);   // 0:10

            q.push(k4);         // 1:7

            k = q.top();
            q.pop();

            REQUIRE(k == k1);   // 0:5

            k = q.top();
            q.pop();

            // batch_id==0 elements must be fully popped before flipping batch_id=0 back to current
            REQUIRE(k.batch_id() != 0); // TODO: Bake this into flip mechanism itself to catch undefined behaviors
            c.flip();
            REQUIRE(c.current_batch() == 0);

            // Now 1:xxx elements are the non-current ones

            q.push(k1);         // 0:5

            REQUIRE(k == k4);   // 1:7

            k = q.top();
            q.pop();

            REQUIRE(k == k3);   // 1:3

            k = q.top();
            q.pop();

            REQUIRE(k == k1);   // 0:5


            REQUIRE(q.empty());
        }
        SECTION("limit finder")
        {
            REQUIRE(helper.would_add_rollover(5, 2, 10) == false);
            REQUIRE(helper.would_add_rollover(5, 7, 10) == true);

            helper.current = 10;    // sync up with q.push(0:10) from above
            helper.emplace(1);      // helper takes deltas, so this is an 11
            helper.emplace(7);      // 0:18
            helper.emplace(5);      // 0:23 -> 1:3

            helper.pop_and_compare(BatchKey(0, 18));
            helper.pop_and_compare(BatchKey(0, 11));
            helper.pop_and_compare(k2); // 0:10
            helper.pop_and_compare(k1); // 0:5
            helper.pop_and_compare(k3); // 1:3
        }
    }
    SECTION("rebaser")
    {
        using namespace embr::internal;

        // DEBT: would prefer to use estd here but cannot
        // since lower estd::chrono::steady_clock aliases out to std::chrono
        constexpr std::chrono::seconds s1(5);
        constexpr std::chrono::seconds s2(10);

        SECTION("intrinsic time_point")
        {
            std::vector<test::rebase::Item> items;
            Rebaser<decltype(items)> rebaser(items);

            items.emplace_back(10);
            items.emplace_back(20);

            rebaser.rebase(5);

            REQUIRE(items[0].event_due() == 5);
            REQUIRE(items[1].event_due() == 15);
        }
        SECTION("intrinsic time_point with pointers")
        {
            std::vector<test::rebase::Item*> items;
            Rebaser<decltype(items)> rebaser(items);

            std::unique_ptr<test::rebase::Item>
                item1(new test::rebase::Item(10)),
                item2(new test::rebase::Item(20));

            items.push_back(item1.get());
            items.push_back(item2.get());

            rebaser.rebase(5);

            REQUIRE(item1->event_due() == 5);
            REQUIRE(item2->event_due() == 15);
        }
        SECTION("chrono time_point detector")
        {
            estd::chrono::steady_clock::time_point t1(s1);

            constexpr bool v1 = is_time_point<decltype(t1)>::value;
            constexpr bool v2 = is_time_point<int>::value;

            REQUIRE(v1 == true);
            REQUIRE(v2 == false);
        }
        SECTION("chrono time_point")
        {
            std::vector<test::rebase::ChronoItem> items;
            Rebaser<decltype(items)> rebaser(items);

            test::rebase::ChronoItem item1, item2;

            item1.t = estd::chrono::steady_clock::time_point(s2);
            item2.t = item1.t + s2;

            items.push_back(item1);
            items.push_back(item2);

            rebaser.rebase(s1);

            auto t0 = items[0].t;
            auto t1 = items[1].t;

            REQUIRE(t0.time_since_epoch() == s1);
            REQUIRE(t1.time_since_epoch() == s1 + s2);
        }
        SECTION("chrono time_point with pointers")
        {
            std::vector<test::rebase::ChronoItem*> items;
            Rebaser<decltype(items)> rebaser(items);

            //auto item1 = std::make_unique<test::rebase::ChronoItem>(s2);  // Not available in c++11
            std::unique_ptr<test::rebase::ChronoItem> item1(new test::rebase::ChronoItem);
            item1->t = estd::chrono::steady_clock::time_point(s2);

            items.push_back(item1.get());

            rebaser.rebase(s1);

            REQUIRE(item1->t.time_since_epoch() == s1);
        }
    }
}
