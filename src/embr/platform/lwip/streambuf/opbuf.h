#include "base.h"

namespace embr { namespace lwip {

#ifndef EMBR_LWIP_PBUF_OSTREAMBUF_GROW_BY
#define EMBR_LWIP_PBUF_OSTREAMBUF_GROW_BY 256
#endif

// Displaces all netbuf abstractions, they are finally gone
// and replaced by more straightforward estd::streambuf capabilities
#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

namespace impl {

template <class TCharTraits, unsigned grow_by = EMBR_LWIP_PBUF_OSTREAMBUF_GROW_BY>
struct opbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    pbuf_current_base<TCharTraits>,
    estd::internal::impl::out_pos_streambuf_base<TCharTraits>
{
    typedef pbuf_streambuf_base<TCharTraits> pbuf_base_type;
    typedef estd::internal::impl::out_pos_streambuf_base<TCharTraits> base_type;
    typedef pbuf_current_base<TCharTraits> pbuf_current_base_type;
    typedef TCharTraits traits_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    typedef Pbuf::pointer pbuf_pointer;
    typedef Pbuf::size_type size_type;

    // Attempts to move to the beginning of the next pbuf in the chain
    bool move_next()
    {
        if(!pbuf_current_base_type::move_next()) return false;

        base_type::seekpos(0);
        return true;
    }

public:

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* pbase() { return this->current_data(); }
    const char_type* pbase() const { return this->current_data(); }
    char_type* pptr() { return pbase() + pos; }
    const char_type* pptr() const { return pbase() + pos; }
    char_type* epptr() const { return pbase() + this->current_size(); }
#else
    char_type* pbase() const { return this->current_data(); }
    char_type* pptr() const { return pbase() + this->pos(); }
    char_type* epptr() const { return pbase() + this->current_size(); }
#endif

private:
    // amount of buffer space left we can write to for this particular pbuf chain portion
    int_type xout_avail() const { return this->current_size() - this->pos(); }

protected:
    // NOTE: Not saving to output sequence, because for most of our implementations, this one
    // included, we treat pbase() buffer and output sequence as the same
    int_type overflow(int_type ch = traits_type::eof());

public:
    int_type sputc(char_type ch)
    {
        int_type _ch = traits_type::to_int_type(ch);
        int_type result = overflow(_ch);
        if(result == traits_type::eof()) return traits_type::eof();
        return _ch;
    }

    // NOTE: Remember, semi-nonstandard, wrapping sputn does all the overflow trickery
    // TODO: Should we be using pbuf_take?  Seems like pbuf_take always does tot_len, which
    //       would make that a no.  But that also would make no sense
    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        int_type avail = xout_avail();
        estd::streamsize remaining = avail - count;
        
        if(remaining < 0)
            count = avail;

        memcpy(pptr(), s, count);
        this->pbump(count);
        return count;
    }

    void shrink()
    {
        pbuf_base_type::pbuf.realloc(
            pbuf_base_type::pbuf.total_length() - xout_avail());
    }

    v1::Pbuf& pbuf() { return pbuf_base_type::pbuf; }

    const internal::Pbuf pbuf() const { return pbuf_base_type::pbuf; }

#ifdef __cpp_variadic_templates
    template <class ...TArgs>
    opbuf_streambuf(TArgs&&... args) :
        pbuf_base_type(std::forward<TArgs>(args)...),
        pbuf_current_base_type(pbuf_base_type::pbuf)
    {
    }
#endif
};


}}

}}