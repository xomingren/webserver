#pragma once

#include <stdint.h>
#include <string>

namespace CurrentThread
{
    // internal
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char* t_threadName;
    void CacheTid();

    inline int Tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            CacheTid();
        }
        return t_cachedTid;
    }

    inline const char* TidString() // for logging
    {
        return t_tidString;
    }

    inline int TidStringLength() // for logging
    {
        return t_tidStringLength;
    }

    inline const char* Name()
    {
        return t_threadName;
    }

    bool IsMainThread();

    void SleepUsec(int64_t usec);  // for testing

    std::string StackTrace(bool demangle);
}  // namespace CurrentThread
