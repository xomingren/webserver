#pragma once

#include <memory>//for shared_ptr

template<typename To, typename From>
inline To implicit_cast(From const& f)
{
    return f;
}

template<typename To, typename From>     // use like this: down_cast<T*>(foo);
inline To down_cast(From* f)                     // so we only accept pointers
{
    if (false)
    {
        implicit_cast<From*, To>(0);
    }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == nullptr || dynamic_cast<To>(f) != nullptr);  // RTTI: debug mode only!
#endif
    return static_cast<To>(f);
}

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
    return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
    return ptr.get();
}

template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
    if (false)
    {
        implicit_cast<From*, To*>(0);
    }

#ifndef NDEBUG
    assert(f == nullptr || dynamic_cast<To*>(get_pointer(f)) != nullptr);
#endif
    return ::std::static_pointer_cast<To>(f);
}

