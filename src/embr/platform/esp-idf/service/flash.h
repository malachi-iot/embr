#pragma once

#include <embr/service.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

struct Flash : embr::SparseService
{
    // DEBT: Diagnostic observer assumes presence of substate(), so
    // doing this temporarily just to satisfy it.  Might consider a
    // special substate to reflect sparse/stateless service.
    constexpr static Substates substate() { return Finished; }

    // TODO: Make embr::Service this_type private so that we don't
    // accidentally pick it up
    typedef Flash this_type;

    constexpr static const char* TAG = "Flash";
    constexpr static const char* name() { return TAG; }

    // DEBT: Didn't get any warnings when this was just EMBR_PROPERTY_RUNTIME
    // -- should we?
    EMBR_SERVICE_RUNTIME_BEGIN(embr::SparseService)

    state_result on_start();

    EMBR_SERVICE_RUNTIME_END

    // DEBT: Make this a macro somewhere
    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;
};

}}

}