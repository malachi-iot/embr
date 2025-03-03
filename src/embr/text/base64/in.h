#pragma once

#include <estd/streambuf.h>

namespace embr { namespace text { inline namespace v1 { namespace impl {

extern const signed char base64de[];

// Adaptation of https://github.com/nkolban/esp32-snippets/blob/master/cloud/GCP/JWT/base64url.cpp

// Not implemented yet
template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Wrapped>
class in_base64_streambuf : public estd::internal::impl::streambuf_base<estd::char_traits<char>>
{
    using base_type = estd::internal::impl::streambuf_base<estd::char_traits<char>>;
    using wrapped_type = estd::remove_reference_t<Wrapped>;

    constexpr static int BASE64DE_FIRST = '+';
    constexpr static int BASE64DE_LAST = 'z';

    Wrapped wrapped_;

    using streamsize = estd::streamsize;
    int8_t s_ = 0;
    char_type out_;

public:
    using typename base_type::traits_type;
    using typename base_type::char_type;

private:
    template <class F>
    void decode(char_type ch, F&& out)
    {
        int c;

        if(ch == '=')   return; // TODO: return 'OK'

        if(ch < BASE64DE_FIRST || ch > BASE64DE_LAST ||
            (c = base64de[ch - BASE64DE_FIRST]) == -1)
            return; // TODO: Return 'invalid'

        switch(s_)
        {
            case 0:
                out_ = ((unsigned)c << 2) & 0xFF;
                break;

            case 1:
                out_ += ((unsigned)c >> 4) & 0x3;
                out(out_);
                //out_ =
                break;

            case 2:
                out_ += ((unsigned)c >> 2) & 0xF;
                out(out_);
                //out_ =
                break;

            case 3:
                out_ = (char_type) c;
                out(out_);
                break;

            default:    break;
        }
    }

public:
};

}}}}
