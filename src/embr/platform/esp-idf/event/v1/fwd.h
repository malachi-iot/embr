#pragma once

#include <esp_event.h>

template <esp_event_base_t event_base, int32_t event_id>
struct embr_esp_event_traits_legacy;

template <class EventEnum>
struct embr_esp_event_base_traits;

template <auto event_id>
struct embr_esp_event_traits_exp;

namespace embr::esp_idf::inline event::inline v1 {

template <auto event_id>
using event_traits = embr_esp_event_traits_exp<event_id>;

template <esp_event_base_t event_base, int32_t event_id>
using event_traits_legacy = embr_esp_event_traits_legacy<event_base, event_id>;

template <auto event_id>
using event_data = const event_traits<event_id>::data_type;

}


// By default, our strongly typed event mechanism does some enforcement of
// EMBR_IDF_EVENT_TRAITS usage.  This reduces that, allowing more use cases
// and greater risk of incorrect casts.
// We want this to be OFF.  During bringup seeing some glitches with the enforcement
// mechanisms, so keeping on for now.
#ifndef FEATURE_EMBR_ESP_EVENT_UNSAFE
#define FEATURE_EMBR_ESP_EVENT_UNSAFE 1
#endif