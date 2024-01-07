#pragma once

// Describes the non-polled/GPIO-isr debouncer.  A highly complicated beast,
// and at present also depends on pre 5.0 gptimer API.
// DEBT: Feature flag needs a better name
#ifndef FEATURE_EMBR_ESP_LEGACY_DEBOUNCE
#define FEATURE_EMBR_ESP_LEGACY_DEBOUNCE 1
#endif

// Describes a special embr::Scheduler mated to esp-idf pre 5.0 gptimer
// By and large one WOULD want this always enabled, but it creates warnings
// due to deprecation of pre 5.0 gptimer
#ifndef FEATURE_EMBR_ESP_TIMER_SCHEDULER
#define FEATURE_EMBR_ESP_TIMER_SCHEDULER 0
#endif
