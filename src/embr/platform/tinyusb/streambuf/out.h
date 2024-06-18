#pragma once

#include <estd/streambuf.h>

namespace embr { namespace tinyusb { inline namespace v1 {

namespace impl {

template <class CharTraits>
struct ocdc_streambuf
{
    using traits_type = CharTraits;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

protected:
    int_type overflow(int_type ch = traits_type::eof())
    {
        return {};
    }

public:
    // DEBT: Feels like a boilerplate that owning streambuf can
    // implement
    int_type sputc(char_type ch)
    {
        const int_type _ch = traits_type::to_int_type(ch);
        int_type result = overflow(_ch);
        if(result == traits_type::eof()) return traits_type::eof();
        return _ch;
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        return {};
    }
};

}

}}}
