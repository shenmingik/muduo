#include <errno.h>
#include <unistd.h>
#include <cstring>

#include "EpollPoller.hpp"
#include "Logger.hpp"
#include "Channel.hpp"

const int k_new = -1;
const int k_added = 1;
const int k_deleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)) //create1 可以防止子进程继承epollfd
      ,
      events_(k_init_eventList_size)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create1 error:%d\n", errno);
    }
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

//epoll_ctl add del mod
void EpollPoller::update_channel(Channel *channel)
{
    //增加、删除
    const int index = channel->index();
    LOG_INFO("func = %s fd = %d events = %d index = %d \n", __FUNCTION__, channel->get_fd(), channel->get_events(), index);

    if (index == k_new || index == k_deleted)
    {
        int sockfd = channel->get_fd();
        if (index == k_new)
        {
            //添加
            channels_[sockfd] = channel;
        }

        channel->set_index(k_added);
        update(EPOLL_CTL_ADD, channel);
    }
    else //注册过
    {
        int sockfd = channel->get_fd();
        if (channel->is_none_event()) //对所有事件都不感兴趣
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(k_deleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//从poller中删除channel
void EpollPoller::remove_channel(Channel *channel)
{
    int sockfd = channel->get_fd();
    int index = channel->index();

    channels_.erase(sockfd);

    LOG_INFO("func = %s fd = %d events = %d index = %d \n", __FUNCTION__, channel->get_fd(), channel->get_events(), index);

    if (index == k_added)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(k_deleted);
}

//epoll_wait
TimeStamp EpollPoller::poll(int timeout, ChannelList *active_channels)
{
    //原为info，但是因为每次都要io执行，影响效率，所有改为debug
    LOG_DEBUG("func = %s => fd total count:%d\n", __FUNCTION__, channels_.size());

    int events_num = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout);
    //记录发生事件错误信息
    int save_errno = errno;

    //记录发生事件的时间
    TimeStamp now(TimeStamp::now());
    if (events_num > 0)
    {
        LOG_DEBUG("%d events happened \n", events_num);
        fill_active_channels(events_num, active_channels);

        //需要扩容
        if (events_num == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (events_num == 0)
    {
        LOG_DEBUG("%s timeout \n", __FUNCTION__);
    }
    else
    {
        //不是外部错误
        if (save_errno != EINTR)
        {
            //重置为原来错误
            errno = save_errno;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }
    //返回发生事件时间点
    return now;
}

//填写活跃的链接
void EpollPoller::fill_active_channels(int events_num, ChannelList *active_channels) const
{
    for (int i = 0; i < events_num; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revent(events_[i].events);
        active_channels->push_back(channel);    //eventloop得到poller给他返回的所有发生事件的channel列表
    }
}

//更新channel，调用epoll_ctl
void EpollPoller::update(int operation, Channel *channel)
{
    int sockfd = channel->get_fd();

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->get_events();
    event.data.fd = sockfd;
    event.data.ptr = channel;

    if (epoll_ctl(epollfd_, operation, sockfd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d \n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
