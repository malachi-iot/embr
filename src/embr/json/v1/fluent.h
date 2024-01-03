#pragma once

#include <estd/type_traits.h>

#include "encoder.h"

namespace embr { namespace json {

inline namespace v1 {

namespace minij {

enum modes
{
    core = 0,
    array,
    normal,
    begin
};

}

template <class Out, class Encoder = encoder<>, int mode = minij::core>
class fluent;

// TODO: Do variant of this where streambuf can be
// value rather than references (optimization)
// DEBT: Use c++20 concept to enforce 'Encoder' signature
template <class TStreambuf, class TBase, class Encoder, int mode_>
class fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder, mode_>
{
    typedef estd::detail::basic_ostream <TStreambuf, TBase> out_type;
    typedef Encoder encoder_type;
    typedef typename estd::remove_cvref<Encoder>::type::options_type options_type;

    typedef fluent<out_type, encoder_type> default_type;
    typedef fluent<out_type, encoder_type, mode_> this_type;
    typedef fluent<out_type, encoder_type, minij::array> array_type;
    typedef fluent<out_type, encoder_type, minij::normal> mode2_type;
    typedef fluent<out_type, encoder_type, minij::begin> begin_type;

    static ESTD_CPP_CONSTEXPR_RET int mode() { return mode_; }

    out_type& out;
    encoder_type json;

public:
    ESTD_CPP_CONSTEXPR_RET fluent(out_type& out, encoder<options_type>& json) :
        out(out),
        json(json)
    {}

    ESTD_CPP_CONSTEXPR_RET fluent(out_type& out) :
        out(out)
    {}

#if __cpp_rvalue_references
    fluent(const fluent&) = default;
    fluent(fluent&&) = default;
#endif

    template <typename T>
    this_type& add(const char* key, T value)
    {
        json.add(out, key, value);
        return *this;
    }

    this_type& begin()
    {
        json.begin(out);
        return *this;
    }

    this_type& begin(const char* key)
    {
        json.begin(out, key);
        return *this;
    }

    this_type& array(const char* key)
    {
        json.array(out, key);
        return *this;
    }

    template <class T>
    this_type& array_item(const T& value)
    {
        json.array_item(out, value);
        return *this;
    }

    this_type& end()
    {
        json.end(out);
        return *this;
    }

    this_type& operator()(const char* key)
    {
        return begin(key);
    }

    this_type& operator()()
    {
        return end();
    }

    template <typename T>
    this_type& operator()(const char* key, const T& value)
    {
        return add(key, value);
    }

    array_type& operator[](const char* key)
    {
        return (array_type&) array(key);
    }

    this_type& operator--(int)
    {
        return end();
    }

/*
 - fascinating, but deeper overloading of () is better
    template <typename T>
    fluent& operator,(T value)
    {
        json.array_item(out, value);
        return *this;
    } */

// almost but not quite
/*
begin_type& operator++(int)
{
    return (begin_type&) *this;
} */


/*
    begin_type* operator->()
    {
        return (begin_type*) this;
    } */

/*
fluent& operator=(const char* key)
{
    return *this;
} */
};

// TODO: Look into that fnptr-like magic that ostream uses for its endl
template <class Out, class Encoder, int mode>
fluent<Out, Encoder, mode>& end(fluent<Out, Encoder, mode>& j)
{
    return j.end();
}

template <class TStreambuf, class TBase, class Encoder>
class fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder, minij::array> :
    public fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder>
{
    typedef fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder>
            base_type;
    typedef fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder,
            minij::array>
            this_type;

public:
    template <class T>
    void array_items(T item)
    {
        base_type::array_item(item);
    }

    template <class T, class ... Args>
    void array_items(T item, Args...args)
    {
        base_type::array_item(item);
        array_items(std::forward<Args>(args)...);
    }

    template <class ... Args>
    base_type& operator()(Args...args)
    {
        array_items(std::forward<Args>(args)...);
        base_type::end();
        return *this;
    }
};

template <class TStreambuf, class TBase, class Encoder>
class fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder, minij::begin> :
    public fluent<estd::detail::basic_ostream<TStreambuf, TBase>, Encoder>
{
public:
    fluent& operator=(const char* key)
    {
        return *this;
    }
};

/*
template <class TOut>
fluent <TOut>& operator<(fluent <TOut>& j, const char* key)
{
    return j.begin(key);
}

// doesn't work
template <class TOut>
fluent <TOut>& operator>(fluent <TOut>& j, fluent <TOut>&)
{
    return j.end();
}
*/

// TODO: Rework this so that encoder can be inline inside the fluent mechanism

template <class TStreambuf, class TBase, class Options>
fluent<estd::detail::basic_ostream<TStreambuf, TBase>, encoder<Options>& >
ESTD_CPP_CONSTEXPR_RET make_fluent(encoder<Options>& json, estd::detail::basic_ostream<TStreambuf, TBase>& out)
{
    return fluent<
        estd::detail::basic_ostream<TStreambuf, TBase>,
        encoder<Options>& >
        (out, json);
}

// DEBT: Double check that ADL doesn't get us into trouble here.  I think it will,
// so plan to rename these to 'fluent_json'
template <class TStreambuf, class TBase>
fluent<estd::detail::basic_ostream<TStreambuf, TBase> >
ESTD_CPP_CONSTEXPR_RET make_fluent(estd::detail::basic_ostream<TStreambuf, TBase>& out)
{
    return fluent<
        estd::detail::basic_ostream<TStreambuf, TBase> >
        (out);
}

}

}}
