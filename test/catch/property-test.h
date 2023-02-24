#pragma once

#include <embr/property/v1/notifier.h>

// Filter is a stateless observer which we expect to morph incoming property changes into its own
// flavor
// Base class of container is optional, just for convenient access to its
// aliaeses
struct Filter1Base : embr::property::v1::PropertyContainer
{
    typedef Filter1Base this_type;

    enum Properties
    {
        BATTERY_LEVEL,
        BATTERY_ALERT
    };

    EMBR_PROPERTIES_SPARSE_BEGIN

        typedef embr::internal::property::traits_base<this_type, int, BATTERY_LEVEL> battery_level;

        template <bool dummy>
        struct lookup<BATTERY_LEVEL, dummy> : battery_level {};

        EMBR_PROPERTY_ID_SPARSE(battery_alert, int, BATTERY_ALERT, "alert");

    EMBR_PROPERTIES_SPARSE_END
};


