#include <catch2/catch_all.hpp>

// Wishful thinking, so far we are pretty linked up to esp-idf's ring buffer
// however, pggcc-33 proves that frequently lambdas can be copied and moved,
// sometimes trivially.  That opens the door for limited support in non-emplace
// scenarios
//#include <embr/internal/delegate_queue.h>

// NOTE: Have a look at thunk-test which is 80%+ overlap

TEST_CASE("delegate queue")
{
}