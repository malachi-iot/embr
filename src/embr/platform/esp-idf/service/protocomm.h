#pragma once

#include <protocomm.h>

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

class Protocomm : embr::service::v1::Service
{
    using this_type = Protocomm;

protected:
    protocomm_t* pc;

    state_result on_stop();

public:
    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

    state_result on_start();

    EMBR_SERVICE_RUNTIME_END
};


}}}}