#pragma once

#include <estd/streambuf.h>
#include "../pbuf.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

namespace impl {

template <class TCharTraits>
class pbuf_streambuf_base
{
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;

public:
    typedef Pbuf::size_type size_type;

protected:
    Pbuf pbuf;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* data() const { return static_cast<char_type*>(pbuf.payload()); }
#endif
    size_type size() const { return pbuf.length(); }

#ifdef __cpp_variadic_templates
        template <class ...TArgs>
        pbuf_streambuf_base(TArgs&&... args) :
                pbuf(std::forward<TArgs>(args)...)
        {
        }
#endif

public:
};



template <class TCharTraits>
class pbuf_current_base
{
    typedef TCharTraits traits_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    typedef PbufBase::pointer pbuf_pointer;
    typedef PbufBase::size_type size_type;

protected:
    PbufBase pbuf_current;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* current_data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* current_data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* current_data() const
    { return static_cast<char_type*>(pbuf_current.payload()); }
#endif
    size_type current_size() const { return pbuf_current.length(); }

    bool move_next()
    {
        pbuf_pointer next = pbuf_current.pbuf()->next;

        if(next == NULLPTR) return false;

        pbuf_current = next;
        return true;
    }

public:
    pbuf_current_base(const PbufBase& pbuf) :
        pbuf_current(pbuf)
    {

    }
};

}}

}}