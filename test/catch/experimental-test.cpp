#include <catch.hpp>

#include <estd/string.h>
#include <estd/string_view.h>

// NOTE: At this time, not yet freertos specific, so test it here in regular GNU area
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/streambuf.hpp>
#include <estd/sstream.h>

#include <embr/bits/bits.hpp>

#include "test-data.h"

using namespace embr::experimental;

template <class TTransport, class TRetryPolicyImpl, class TTimer>
std::allocator<typename RetryManager<TTransport, TRetryPolicyImpl, TTimer>::QueuedItem>
        RetryManager<TTransport, TRetryPolicyImpl, TTimer>::stub;

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


#define EMBR_CPP_VALUE_TYPE(T) \
typedef T value_type;           \
typedef const value_type& const_reference; \
typedef T* pointer;

// NOTE: scheduler defaults to a flavor of 'greater'
// remember std::priority_queue 'less' results in a max queue
template <class T, class TCompare = estd::less<typename T::key_type> >
struct BatchCompareTraits
{
    EMBR_CPP_VALUE_TYPE(T)

    typedef typename value_type::key_type key_type;
    typedef TCompare key_compare;

    static constexpr int batch_id(const_reference v) { return v.batch_id(); }
    static constexpr const key_type& key(const_reference v) { return v.key(); }
};

template <class T, class TTraits = BatchCompareTraits<T> >
struct BatchCompare : TTraits::key_compare
{
    EMBR_CPP_VALUE_TYPE(T)

    typedef TTraits traits_type;
    typedef typename traits_type::key_compare key_compare;

    int current_batch_ = 0;

    void flip()
    {
        current_batch_ = !current_batch_;
    }

    int current_batch() const { return current_batch_; }

    bool operator ()(const_reference left, const_reference right)
    {
        // TODO: We'll need to return to this for scheduler
        //time_point l_tp = impl_type::get_time_point(left);
        //time_point r_tp = impl_type::get_time_point(right);
        //return l_tp > r_tp;

        bool is_left_current_batch = current_batch_ == traits_type::batch_id(left);
        bool is_right_current_batch = current_batch_ == traits_type::batch_id(right);

        // left is current, right is not, therefore left is greater
        if(is_left_current_batch && !is_right_current_batch) return true;

        // right is current, left is not, therefore right is greater
        if(!is_left_current_batch && is_right_current_batch) return false;

        // both right and left are in the same batch, so regular key compare ensues

        return key_compare().operator()(traits_type::key(left), traits_type::key(right));
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

std::ostream& operator << ( std::ostream& os, const BatchKey& value ) {
    os << value.batch_id() << ':' << value.key();
    return os;
}

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("Retry v3")
    {
        // stringbufs still a mess, so using span variety
        //typedef estd::experimental::ostringstream<128> ostream_type;
        typedef estd::experimental::ospanstream ostream_type;
        typedef estd::experimental::ispanstream istream_type;

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

                estd::copy_n(le_example1, 4, d.value());

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
        BatchCompare<BatchKey> c;
        estd::layer1::priority_queue<BatchKey, 10, BatchCompare<BatchKey> > q(c);

        constexpr BatchKey k1(0, 5), k2(0, 10), k3(1, 3), k4(1, 7);

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
            c.flip();

            q.push(k3);         // 1:3

            k = q.top();
            q.pop();

            REQUIRE(k == k3);   // 1:3

            k = q.top();
            q.pop();

            REQUIRE(k == k2);   // 0:10

            q.push(k4);         // 1:7

            k = q.top();
            q.pop();

            REQUIRE(k == k4);   // 1:7

            k = q.top();

            REQUIRE(k == k1);   // 0:5

            // batch_id==0 elements must be fully popped before flipping batch_id=0 back to active
            // that means batch comparisons may be backwards
            c.flip();
        }
    }
}
