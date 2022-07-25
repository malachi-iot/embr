#include <estd/internal/platform.h>

#ifdef FEATURE_PRAGMA_PUSH_MACRO
#pragma push_macro("abs")
#pragma push_macro("max")
#pragma push_macro("min")
#pragma push_macro("word")
#undef abs
#undef max
#undef min
#undef word
#endif
