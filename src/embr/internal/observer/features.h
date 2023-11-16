#pragma once

#include <estd/internal/platform.h>

#ifdef __cpp_variadic_templates
// DEBT: There are limited forms of this observer code which can work with
// c++03 (namely, lacking variadic templates), but at present we
// require c++11 (or at least variadic templates) for this feature to come online
#define FEATURE_EMBR_OBSERVER 1
#endif
