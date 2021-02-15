#pragma once

#include "NonCopyable.hpp"
class InetAddress;

class Socket : NonCopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}

    ~Socket();

    int get_fd() { return sockfd_; }

    void bind_address(const InetAddress &loacladdr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdown_write();

    void set_tcp_noDelay(bool on);
    void set_reuseAddr(bool on);
    void set_reusePort(bool on);
    void set_keepAlive(bool on);

private:
    const int sockfd_;
};