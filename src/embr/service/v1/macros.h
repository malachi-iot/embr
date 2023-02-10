#pragma once

#include "../../property/v1/macros.h"

#define EMBR_SERVICE_RUNTIME_BEGIN(base_) EMBR_PROPERTY_RUNTIME_BEGIN(base_)    \
    using base_type::state;                \
    using base_type::configuring;          \
    using base_type::configured;           \
    using base_type::progress;

#define EMBR_SERVICE_RUNTIME_END    EMBR_PROPERTY_RUNTIME_END

