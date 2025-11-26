// EXPERIMENTAL, and not working (see service.h notes)
#define EMBR_IDF_PROP_TRAITS2(ns, origin, event_id, type) \
EMBR_IDF_PROP_TRAITS(ns::origin::event_base, \
    ns::event_id,    \
    decltype(std::declval<ns::origin>().accessor()), \
    type)

