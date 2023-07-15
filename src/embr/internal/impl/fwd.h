#pragma once

// DEBT: namespace needs work
namespace embr { namespace internal { namespace scheduler { namespace impl {

// Deduces int_type and duration from more complicated TTimePoints
template <typename TTimePoint>
struct TimePointTraits;

template <typename TTimePoint, class TTimePointTraits = TimePointTraits<TTimePoint> >
struct ReferenceBase;

#if __cpp_concepts
// TODO: Create control structure concept
// DEBT: Would be very useful to have traits concept
#endif

///
/// \tparam T control structure
/// \tparam TTimePoint
/// \tparam Enabled
template <class T, class TTimePoint = decltype(T().event_due()), class Enabled = void>
struct Reference;

// DEBT: Phase this out in favor of below tag::Reference
struct ReferenceBaseTag {};

namespace tag {

struct Function {};

struct Traditional {};

struct Reference {};

}


}}}}