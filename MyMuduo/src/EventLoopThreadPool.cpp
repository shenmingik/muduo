#include "EventLoopThreadPool.hpp"
#include "EventLoopThread.hpp"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, const string &name)
    : baseloop_(baseloop), name_(name), started_(false), thread_nums_(0), next_(0)
{
}
EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &callback)
{
    started_ = true;
    //整个服务端只有baseloop，也就是mainreactor
    if (thread_nums_ == 0)
    {
        callback(baseloop_);
    }
    else
    {
        for (int i = 0; i < thread_nums_; ++i)
        {
            char buffer[name_.size() + 32] = {0};
            snprintf(buffer, sizeof(buffer), "%s %d", name_.c_str(), i);
            EventLoopThread *t = new EventLoopThread(callback, buffer);
            threads_.push_back(unique_ptr<EventLoopThread>(t));
            loops_.push_back(t->start_loop()); //底层开始创建线程，并绑定一个新的eventloop，返回其地址
        }
    }
}

//如果工作在多线程，baseloop默认轮询方式分配channel给subloop
EventLoop *EventLoopThreadPool::get_nextEventLoop()
{
    EventLoop *loop = baseloop_;

    if (!loops_.empty())
    {
        loop = loops_[next_++];

        //轮询转完一圈，肥来
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

//返回所有loop，如果没有设置子loop，则返回baseloop，设置了就返回所有子loop
vector<EventLoop *> EventLoopThreadPool::get_allLoops()
{
    if (loops_.empty())
    {
        return vector<EventLoop *>(1, baseloop_);
    }
    else
    {
        return loops_;
    }
}