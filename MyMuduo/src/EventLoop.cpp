#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include "EventLoop.hpp"
#include "Logger.hpp"

//防止一个线程创建多个eventloop
__thread EventLoop *t_loop_in_thisTherad = nullptr;

//定义默认的poller io复用超时时间
const int k_poll_timeout = 10000;

//创建wakeupfd，用来notify subreactor来处理新连接channel
int CreateEventfd()
{
    //非阻塞且不被子进程继承
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }

    return event_fd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), calling_pending_functors_(false), threadId_(Current_thread::tid()), poller_(Poller::new_defaultPoller(this)), wakeup_fd(CreateEventfd()), wakeup_channel_(new Channel(this, wakeup_fd))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loop_in_thisTherad)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", this, threadId_);
    }
    else
    {
        t_loop_in_thisTherad = this;
    }

    //设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeup_channel_->set_readcallback(bind(&EventLoop::handle_read, this));
    //每个eventloop都将监听wakechannel的EPOLLIN读事件
    wakeup_channel_->enable_reading();
}
EventLoop::~EventLoop()
{
    wakeup_channel_->dis_enable_all();
    wakeup_channel_->remove();
    close(wakeup_fd);

    t_loop_in_thisTherad = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        active_channels.clear();
        //监听两类fd 一种是client的fd，一种wakeup的fd
        poll_return_time_ = poller_->poll(k_poll_timeout, &active_channels);

        for (Channel *channel : active_channels)
        {
            //Poller监听哪些channel发生事件了，然后上报给eventloop，通知channel处理事件
            channel->handle_event(poll_return_time_);
        }

        //执行当前EventLoop事件循环需要处理的回调操作
        do_pending_functors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
    looping_ = false;
}

//退出事件循环
void EventLoop::quit()
{
    quit_ = true;

    //在其他线程中，调用的quit（在sub中调用main的loop）
    if (!is_in_loopThread() || calling_pending_functors_)
    {
        wakeup();
    }
}

//在当前loop中执行cb
void EventLoop::run_in_loop(Functor cb)
{
    //在当前的loop线程中执行回调
    if (is_in_loopThread())
    {
        cb();
    }
    else //在其他线程执行cb，唤醒loop所在线程执行cb
    {
        queue_in_loop(cb);
    }
}

//把cb放入队列中,唤醒loop所在线程执行cb（pending_functor）
void EventLoop::queue_in_loop(Functor cb)
{
    {
        unique_lock<mutex> lock(functor_mutex_);
        pending_Functors_.emplace_back(cb);
    }

    //唤醒相应的，需要执行上面回调操作的loop线程
    //calling_pending_functors是指 当前loop正在执行回调，但是loop又有了新的回调
    if (!is_in_loopThread() || calling_pending_functors_)
    {
        wakeup(); //唤醒loop所在线程
    }
}

//唤醒loop所在线程
//向wakeupfd中写一个数据,wakeupchannel发生读事件，会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

//poller的方法
void EventLoop::update_channel(Channel *channel)
{
    poller_->update_channel(channel);
}
void EventLoop::remove_channel(Channel *channel)
{
    poller_->remove_channel(channel);
}
bool EventLoop::has_channel(Channel *channel)
{
    return poller_->has_channel(channel);
}

void EventLoop::handle_read() //wake up
{
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd, &one, sizeof(int));

    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop: handleRead() reads %lu bytes instead of 8", n);
    }
}

//执行回调
void EventLoop::do_pending_functors()
{
    vector<Functor> functors;
    calling_pending_functors_ = true;

    //减少持有锁的时间
    {
        lock_guard<mutex> lock(functor_mutex_);
        functors.swap(pending_Functors_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }
    calling_pending_functors_ = false;
}
