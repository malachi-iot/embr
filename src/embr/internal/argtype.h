#pragma once

// Was experimental, but works well enough.  So upgraded to internal
// namespace, but not really used directly by embr

namespace embr { namespace internal {

// Guidance for argument type deduction from:
// https://stackoverflow.com/questions/22632236/how-is-possible-to-deduce-function-argument-type-in-c
// https://stackoverflow.com/questions/8711855/get-lambda-parameter-type
// https://github.com/steinwurf/boost/blob/master/boost/type_traits/function_traits.hpp
template <class F> struct ArgType;

template <class R, class A1>
struct ArgType<R(*)(A1)>
{
    typedef A1 arg1;
    typedef R result_type;
    typedef R type(A1);
};

template <class R, class C, class A1>
struct ArgType<R(C::*)(A1)>
{
    typedef A1 arg1;
    typedef R result_type;
    typedef R type(A1);
};

template <class R, class C, class A1>
struct ArgType<R(C::*)(A1) const>
{
    typedef A1 arg1;
    typedef R result_type;
    typedef R type(A1);
};

template <class T>
typename ArgType<T>::type* ArgHelper(T);



}}
