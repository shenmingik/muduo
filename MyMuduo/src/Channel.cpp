#include <sys/epoll.h>
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

const int Channel::k_none_event_ = 0;
const int Channel::k_read_event_ = EPOLLIN | EPOLLPRI;
const int Channel::k_write_event_ = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), real_events_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

//fd得到poller通知以后，根据具体发生的事件，调用相应的回调
void Channel::handle_event(TimeStamp receive_time)
{
    if (tied_)
    {
        shared_ptr<void> guard = tie_.lock(); //提升为强智能指针
        if (guard)
        {
            handle_event_withGuard(receive_time);
        }
    }
    else
    {
        handle_event_withGuard(receive_time);
    }
}

//防止channel被remove掉，channel还在执行回调
void Channel::tie(const shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

//在channel所属的eventloop中删除自己
void Channel::remove()
{
    loop_->update_channel(this);
}

//与poller更新fd所感兴趣事件
void Channel::update()
{
    //通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->update_channel(this);
}

//根据发生的具体事件调用相应的回调操作
void Channel::handle_event_withGuard(TimeStamp receive_time)
{
    LOG_INFO("channel handleEvent revents:%d\n", real_events_);

    //是断开连接
    if ((real_events_ & EPOLLHUP) && !(real_events_ & EPOLLIN))
    {
        if (close_callback_)
        {
            close_callback_();
        }
    }

    //发生错误
    if (real_events_ & EPOLLERR)
    {
        if (error_callback_)
        {
            error_callback_();
        }
    }

    //读事件
    if (real_events_ & (EPOLLIN | EPOLLPRI))
    {
        if (read_callback_)
        {
            read_callback_(receive_time);
        }
    }

    //写事件
    if (real_events_ & EPOLLOUT)
    {
        if (write_callback_)
        {
            write_callback_();
        }
    }
}