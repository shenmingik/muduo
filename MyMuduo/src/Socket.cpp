#include <strings.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "Socket.hpp"
#include "Logger.hpp"
#include "InetAddress.hpp"

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bind_address(const InetAddress &loacladdr)
{
    if (bind(sockfd_, (sockaddr *)loacladdr.get_sockaddr(), sizeof(sockaddr_in)) != 0)
    {
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}

void Socket::listen()
{
    if (::listen(sockfd_, 1024) != 0)
    {
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /*
    *   1.accept 参数不合法
    *   2.对返回的connfd没有设置非阻塞
    */
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    socklen_t len = sizeof(addr);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->set_sockaddr(addr);
    }
    return connfd;
}

void Socket::shutdown_write()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdown write error");
    }
}

void Socket::set_tcp_noDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::set_reuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::set_reusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::set_keepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}