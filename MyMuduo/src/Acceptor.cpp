#include <sys/types.h>
#include <sys/socket.h>
#include "InetAddress.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "Logger.hpp"

static int create_nonblock_sockfd()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("%s : %s :%d listen socket create error : %d!\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenaddr, bool reuseport)
    : loop_(loop), accept_socket_(create_nonblock_sockfd()), accept_channel_(loop, accept_socket_.get_fd()), listenning_(false)
{
    accept_socket_.set_reuseAddr(true);
    accept_socket_.set_reusePort(true);
    accept_socket_.bind_address(listenaddr);

    accept_channel_.set_readcallback(bind(&Acceptor::handle_read, this));
}

Acceptor::~Acceptor()
{
    accept_channel_.dis_enable_all();
    accept_channel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    accept_socket_.listen();

    //借助poller进行监听
    accept_channel_.enable_reading();
}

//listenfd 有事件发生，即新用户连接
void Acceptor::handle_read()
{
    InetAddress peeraddr;

    int connfd = accept_socket_.accept(&peeraddr);
    if (connfd > 0)
    {
        if (new_connetion_callback_)
        {
            new_connetion_callback_(connfd, peeraddr); //轮询找到subloop，唤醒，分发当前新客户端的channel
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s : %s :%d accept error : %d!\n", __FILE__, __FUNCTION__, __LINE__, errno);
        //进程没有可用的fd
        if (errno == EMFILE)
        {
            LOG_ERROR("%s : %s :%d sockfd reached limit error : %d!\n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
}
