// EXPERIMENTAL, and not working (see service.h notes)
#define EMBR_IDF_PROP_TRAITS2(ns, origin, event_id, type) \
EMBR_IDF_PROP_TRAITS(ns::origin::event_base, \
    ns::event_id,    \
    decltype(std::declval<ns::origin>().accessor()), \
    type)

#define EMBR_IDF_PROP_TRAITS_NS(ns, event_base, event_id, type, origin) \
EMBR_IDF_PROP_TRAITS(ns::event_base, ns::event_id, type, ns::origin)

#define EMBR_IDF_PROP_DECLARE(event_id, name) \
    using name ## _ ## type = ::embr::esp_idf::prop::v1::property_legacy<event_base, event_id>;  \
    using name ## _event_data = typename name ## _ ## type::event_data; \
    /* experimental */ static constexpr const char* prop ## _ ## name ## _id = #name;

#define EMBR_IDF_PROP_PROVIDER(_event_base, origin)    \
    static constexpr esp_event_base_t event_base = _event_base; \
    static constexpr const char* origin_name = #origin;


#define EMBR_IDF_PROP_TRAITS(event_base, event_id, _type, origin) \
template <> \
struct embr_esp_event_traits_legacy<event_base, event_id> \
{ \
    EMBR_ESP_EVENT_TRAITS_BODY(event_base, event_id) \
    static constexpr bool is_property = true; \
    using type = ::embr::esp_idf::prop::property_event_data<_type, origin>; \
    static constexpr const char* type_name = "property<_type, origin>"; \
    /* experimental */ static constexpr int32_t changing_id = id + 10000; \
    /* experimental */ static constexpr const char* property_name = #origin ".TBD"; \
};


