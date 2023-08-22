#include "protocomm.h"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
auto Protocomm::runtime<TSubject, TImpl>::on_start() -> state_result
{
    pc = protocomm_new();

    if(base_type::pc == nullptr) return { Error, ErrMemory };

    return state_result::started();
}


auto Protocomm::on_stop() -> state_result
{
    protocomm_delete(pc);

    return { Stopped, Finished };
}

}}}}