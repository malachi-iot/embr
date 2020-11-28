#pragma once

#include <estd/streambuf.h>
#include <estd/optional.h>

#include "obsolete/netbuf_streambuf.h"

// At time of writing, FEATURE_ESTD_IOSTREAM_STRICT_CONST is invented.  It's more experimental, since it's not
// fully functional.  The idea is that additional const-ness than stock std performs for the pbase, pptr, etc.
// could be useful.  This may have additional consequence when we get into more advanced memory buffers which
// can't be const'd up as much (virtual memory, etc - any memory which has side effects when utilizing it)

namespace embr {

namespace experimental {

namespace streambuf {

namespace event {

// FIX: using enums this way IIRC is a C++11 only thing.  Even though subject-observer largely is that, I think
// this advanced enum-as-a-scopable-type thing doesn't come with the C++03 'preview' of C++11 features.  Needs testing,
// it certainly seems like that one woulda been an easier one for them to do
enum phase
{
    begin,
    end
};

enum type
{
    sbumpc,
    sgetn,
    sputn,
    pubseekoff
};

template <class TChar>
struct span_event_base
{
    estd::span<TChar> buffer;

    span_event_base(estd::span<TChar> buffer) : buffer(buffer) {}
};

template <class TChar>
struct char_event_base
{
    TChar ch;
};


template <typename TChar, typename TEventType, TEventType event, phase phase = phase::end>
struct generic_event_base;

template <class TChar, type, phase phase = phase::end>
struct event;// : generic_event_base<TChar, type, event, phase> {};

template <class TChar, phase phase>
struct event<TChar, type::sbumpc, phase> // : generic_event_base<TChar, event_type, event_type::sbumpc, phase>
{

};


//template <typename TChar>
struct observer_base
{
    /*
    template <type t, phase p>
    using e = event<TChar, t, p>; */

    typedef embr::experimental::streambuf::event::type type;
    typedef embr::experimental::streambuf::event::phase phase_type;
};


template <class TChar, phase phase>
struct event<TChar, type::sgetn, phase>
{

};

template <class TChar, phase phase>
struct event<TChar, type::sputn, phase>
{

};

template <class TChar>
struct sget2 : event<TChar, type::sgetn, phase::end> {};


template <class TChar, phase phase = phase::end>
struct sput : span_event_base<TChar>
{
    typedef span_event_base<TChar> base_type;
    typedef TChar char_type;

    int retval;

    sput(estd::span<TChar> buffer) : base_type(buffer) {}

    sput(char_type* data, int size, int retval = -1) :
            base_type(estd::span<char_type>(data, size)),
            retval(retval) {}
};



template <class TChar, phase phase = phase::end>
struct sget : span_event_base<TChar>
{
    typedef span_event_base<TChar> base_type;

    sget(estd::span<TChar> buffer) : base_type(buffer) {}
};



}

}

// wrapper of sorts which fires off various events via TSubject during streambuf
// operations
template <class TStreambuf, class TSubject>
class subject_streambuf
{
    TSubject subject;
    TStreambuf streambuf;

    //using embr::experimental::_streambuf::event::event;

    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;

    typedef typename streambuf::event::type event_type;
    typedef typename streambuf::event::phase phase;

public:

    // consider doing this all the way down
    //typedef typename estd::remove_const<typename streambuf_type::char_type>::type char_type;
    typedef typename streambuf_type::char_type char_type;
    typedef typename streambuf_type::traits_type traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;
    typedef estd::streamsize streamsize;

    int_type sbumpc()
    {
        using streambuf::event::event;

        subject.notify(event<char_type, event_type::sbumpc, phase::begin>());

        int_type ret = streambuf.sbumpc();

        subject.notify(event<char_type, event_type::sbumpc, phase::end>());

        return ret;
    }

    streamsize sputn(const char_type *s, streamsize count)
    {
        using streambuf::event::event;
        using streambuf::event::sput;

        subject.notify(sput<char, phase::begin>((char*)s, count));

        streamsize ret = streambuf.sputn(s, count);

        subject.notify(sput<char, phase::end>((char*)s, count));
        subject.notify(event<char, event_type::sputn>());

        return ret;
    }

    streamsize sgetn(char_type *s, streamsize count)
    {
        using streambuf::event::event;
        using streambuf::event::sget;

        streamsize ret = streambuf.sgetn(s, count);

        estd::span<char_type> buffer(s, count);

        sget<char_type> e { buffer };
        event<char_type, event_type::sgetn, phase::end> e2;
        //sget<char_type> e3;

        subject.notify(e);
        subject.notify(e2);

        return ret;
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
        template <class ...TArgs>
        subject_streambuf(TSubject& subject, TArgs&&... args) :
                subject(subject),
                streambuf(std::forward<TArgs>(args)...) {}
#endif
};

}

}
