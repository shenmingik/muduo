#include <sys/epoll.h>
#include <vector>
#include "Poller.hpp"

using EventList = vector<epoll_event>;


class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    TimeStamp poll(int timeout, ChannelList *active_channels) override;

    void update_channel(Channel *channel);
    void remove_channel(Channel *channel);

private:
    //填写活跃的链接
    void fill_active_channels(int events_num, ChannelList *active_channels) const;
    //更新channel，调用epoll_ctl
    void update(int operation, Channel *channel);

private:
    static const int k_init_eventList_size = 16;

private:
    int epollfd_;
    EventList events_;
};