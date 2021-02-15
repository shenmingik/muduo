#pragma once
#include<unistd.h>
#include<sys/syscall.h>

namespace Current_thread
{
    extern __thread int t_cachedTid;

    void cache_tid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0,0))
        {
            cache_tid();
        }
        return t_cachedTid;
    }
}