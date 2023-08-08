#pragma once

// Describes the non-polled/GPIO-isr debouncer.  A highly complicated beast,
// and at present also depends on pre 5.0 gptimer API.
// DEBT: Feature flag needs a better name
#ifndef EMBR_ESP_LEGACY_DEBOUNCE
#define EMBR_ESP_LEGACY_DEBOUNCE 1
#endif
