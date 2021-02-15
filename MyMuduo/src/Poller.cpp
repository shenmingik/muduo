#include "Poller.hpp"

Poller::Poller(EventLoop *loop)
    : owner_loop_(loop)
{
}

//判断channel是否在当前poller中
bool Poller::has_channel(Channel *channel) const
{
    auto it = channels_.find(channel->get_fd());
    return it != channels_.end() && it->second == channel;
}
