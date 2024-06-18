#pragma once

#include "encoder.h"

// DEBT: For endl, but endl itself should have a declaration in iosfwd
#include <estd/ostream.h>

namespace embr { namespace json {

inline namespace v1 {

template <class Options>
template <class Streambuf, class Base>
inline void encoder<Options>::do_eol(estd::detail::basic_ostream<Streambuf, Base>& out)
{
    if (options_type::use_eol()) out << estd::endl;
}

}

}}
