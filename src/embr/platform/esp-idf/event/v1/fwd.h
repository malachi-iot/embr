#pragma once

// By default, our strongly typed event mechanism does some enforcement of
// EMBR_IDF_EVENT_TRAITS usage.  This reduces that, allowing more use cases
// and greater risk of incorrect casts.
// We want this to be OFF.  During bringup seeing some glitches with the enforcement
// mechanisms, so keeping on for now.
#ifndef FEATURE_EMBR_ESP_EVENT_UNSAFE
#define FEATURE_EMBR_ESP_EVENT_UNSAFE 1
#endif