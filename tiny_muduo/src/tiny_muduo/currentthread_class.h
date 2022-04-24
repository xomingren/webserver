#pragma once

#include <stdint.h>
#include <string>

namespace CurrentThread
{
    // internal
    extern __thread unsigned int t_cachedtid;
    extern __thread char t_tidstring[32];
    extern __thread int t_tidstringlength;
    extern __thread const char* t_threadname;
    void CacheTid();

    inline unsigned int Tid()
    {
        if (__builtin_expect(t_cachedtid == 0, 0))
        {
            CacheTid();
        }
        return t_cachedtid;
    }

    inline const char* TidString() // for logging
    {
        return t_tidstring;
    }

    inline int TidStringLength() // for logging
    {
        return t_tidstringlength;
    }

    inline const char* Name()
    {
        return t_threadname;
    }

    bool IsMainThread();

    void SleepMicroSeconds(int64_t microsec);  // for testing, microsec

    std::string StackTrace(bool demangle);
}  // namespace CurrentThread
