#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include <functional>

#include "Logger.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"

using namespace placeholders;

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d tcpconnection is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const string name, int sockfd, const InetAddress &localaddr, const InetAddress &peeraddr)
    : loop_(CheckLoopNotNull(loop)), name_(name), state_(k_connecting), reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)), localaddr_(localaddr), peeraddr_(peeraddr), highwater_mark_(64 * 1024 * 1024)
{
    //给channel设置相应的回调函数，poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数s
    channel_->set_readcallback(bind(&TcpConnection::handle_read, this, _1));
    channel_->set_writecallback(bind(&TcpConnection::handle_write, this));
    channel_->set_closecallback(bind(&TcpConnection::handle_close, this));
    channel_->set_errorcallback(bind(&TcpConnection::handle_error, this));

    LOG_INFO("tcp connection::ctor[%s] at fd = %d\n", name_.c_str(), sockfd);

    socket_->set_keepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("tcp connection::dtor[%s] at fd = %d state = %d \n", name_.c_str(), channel_->get_fd(), (int)state_);
}

//发送数据
void TcpConnection::send(const string &buf)
{
    //必须是连接状态
    if (state_ == k_connected)
    {
        //在所属loop所属的线程
        if (loop_->is_in_loopThread())
        {
            send_inLoop(buf);
        }
        else
        {
            loop_->run_in_loop(bind(&TcpConnection::send_inLoop, this, buf));
        }
    }
}

//发送数据，应用写得快，内核发送的慢，我们需要把待发送数据写入缓冲区，而且设置了水位回调
void TcpConnection::send_inLoop(const string &buf)
{
    ssize_t nwrote = 0;
    size_t remaining = buf.size(); //未发送完数据的长度
    bool fault_error = false;

    //之前调用了shutdown
    if (state_ == k_disconnected)
    {
        LOG_ERROR("disconnected,give up writing!\n");
        return;
    }

    //channel第一次开始写数据，而且缓冲区无数据
    if (!channel_->is_writting() && output_buffer_.readable_bytes() == 0)
    {
        nwrote = write(channel_->get_fd(), buf.c_str(), buf.size());
        if (nwrote >= 0)
        {
            //发送成功
            remaining = buf.size() - nwrote;

            if (remaining == 0 && write_complete_callback_)
            {
                //数据一次发送完毕
                loop_->queue_in_loop(bind(write_complete_callback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            //错误不是资源不可用
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("tcp connection::send in loop!\n");
                //如果收到连接重置请求
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    fault_error = true;
                }
            }
        }
    }
    //当前write没有把数据全部发送出去，剩余数据需要保存到缓冲区，
    //然后给channel注册epollout，poller发现tcp缓冲区仍有数据，会调用handle write
    if (!fault_error && remaining > 0)
    {
        //目前发送缓冲区剩余的待发送的长度
        size_t oldlen = output_buffer_.readable_bytes();
        if (oldlen + remaining >= highwater_mark_ && oldlen < highwater_mark_)
        {
            loop_->queue_in_loop(bind(highwater_callback_, shared_from_this(), oldlen + remaining));
        }
        output_buffer_.append(buf.c_str() + nwrote, remaining);
        if (!channel_->is_writting())
        {
            channel_->enable_writing(); //注册channel写事件，否则poller不会通知
        }
    }
}

//断开连接
void TcpConnection::shutdown()
{
    if (state_ == k_connected)
    {
        set_state(k_disconnecting);
        loop_->run_in_loop(bind(&TcpConnection::shutdown_inLoop, this));
    }
}

void TcpConnection::shutdown_inLoop()
{
    //当前outputbuffer 中数据 全部发送完
    if (!channel_->is_writting())
    {
        socket_->shutdown_write(); //关闭写端      后续调用handle close
    }
}

//建立连接
void TcpConnection::establish_connect()
{
    set_state(k_connected);
    channel_->tie(shared_from_this());
    channel_->enable_reading(); //向poller注册channel的epollin事件

    //新连接建立
    connection_callback_(shared_from_this());
}

//销毁连接
void TcpConnection::destory_connect()
{
    if (state_ == k_connected)
    {
        set_state(k_disconnected);
        channel_->dis_enable_all(); //把channel所有感兴趣的事件，从poller中delete
    }
    channel_->remove(); //从poller中删除掉
}

void TcpConnection::handle_read(TimeStamp receive_time)
{
    //从channel中读数据
    int save_errno = 0;
    ssize_t n = input_buffer_.readfd(channel_->get_fd(), &save_errno);
    if (n > 0)
    {
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调操作 onmessage
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    else if (n == 0) //客户端断开连接
    {
        handle_close();
    }
    else //错误
    {
        errno = save_errno;
        LOG_ERROR("tcp connection::handle read\n");
        handle_error();
    }
}

void TcpConnection::handle_write()
{
    //如果channel对写事件感兴趣
    if (channel_->is_writting())
    {
        int save_errno = 0;
        ssize_t n = output_buffer_.writefd(channel_->get_fd(), &save_errno);
        if (n > 0)
        {
            output_buffer_.retrieve(n);
            //发送完成
            if (output_buffer_.readable_bytes() == 0)
            {
                channel_->dis_enable_writing();
                if (write_complete_callback_)
                {
                    loop_->queue_in_loop(bind(write_complete_callback_, shared_from_this()));
                }

                //如果正在关闭
                if (state_ == k_disconnecting)
                {
                    shutdown_inLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("tcp connection::handle write\n");
        }
    }
    else //不可写
    {
        LOG_ERROR("tcp connection fd=%d is down,no more send\n", channel_->get_fd());
    }
}

void TcpConnection::handle_close()
{
    LOG_INFO("fd=%d state=%d", channel_->get_fd(), (int)state_);

    set_state(k_disconnected);
    channel_->dis_enable_all();

    TcpConnectionPtr connection_ptr(shared_from_this());

    //执行关闭连接的回调
    connection_callback_(connection_ptr);
    //关闭连接的回调
    close_callback_(connection_ptr);
}

void TcpConnection::handle_error()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;

    if (getsockopt(channel_->get_fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        //出错
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("tcp connection handle error name:%s  SO_ERROR:%d\n", name_.c_str(), err);
}
