#include "CurrentThread.hpp"

namespace Current_thread
{
    __thread int t_cachedTid = 0;

    void cache_tid()
    {
        if (t_cachedTid == 0)
        {
            //通过Linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(syscall(SYS_getpid));
        }
    }
} // namespace Current_threaad