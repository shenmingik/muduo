#include <cstdlib>
#include "Poller.hpp"
#include "EpollPoller.hpp"

//获取这个事件循环的poller
Poller *Poller::new_defaultPoller(EventLoop *loop)
{
    if (getenv("MUDUO_USE_POLL"))
    {
        return nullptr; //poll
    }
    else
    {
        return new EpollPoller(loop); //epoll
    }
}
