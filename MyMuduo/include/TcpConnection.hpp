#pragma once

#include <string>
#include <atomic>
#include <memory>
#include "NonCopyable.hpp"
#include "InetAddress.hpp"
#include "TimeStamp.hpp"
#include "Callbacks.hpp"
#include "Buffer.hpp"

class Channel;
class EventLoop;
class Socket;

using namespace std;

/*
 *  tcpserver 通过accepter，有一个新用户连接，accep拿到connfd
 * tcpconnection设置channel相应回调，poller调用回调 
*/

class TcpConnection : NonCopyable, public enable_shared_from_this<TcpConnection>
{
private:
    enum StateE
    {
        k_disconnected,
        k_connecting, //正在连接
        k_connected,  //已连接
        k_disconnecting,
    };

public:
    TcpConnection(EventLoop *loop, const string name, int sockfd, const InetAddress &localaddr, const InetAddress &peeraddr);
    ~TcpConnection();

    EventLoop *get_loop() const { return loop_; }

    const string &get_name() const { return name_; }

    const InetAddress &get_localaddr() const { return localaddr_; }

    const InetAddress &get_peeraddr() const { return peeraddr_; }

    bool connected() { return state_ == k_connected; }

    void set_state(StateE state) { state_ = state; }

    //发送数据
    void send(const string& buf);

    //断开连接
    void shutdown();

    //建立连接
    void establish_connect();

    //销毁连接
    void destory_connect();

    //设置回调
    void set_close_callback(const CloseCallback &callback) { close_callback_ = callback; }
    void set_highwater_callback(const HighWaterMarkCallback &callback) { highwater_callback_ = callback; }

    void set_connection_callback(const ConnectionCallback &callback) { connection_callback_ = callback; }
    void set_message_callback(const MessageCallback &callback) { message_callback_ = callback; }
    void set_write_complete_callback(const WriteCompleteCallback &callback) { write_complete_callback_ = callback; }

private:
    void handle_read(TimeStamp receive_time);
    void handle_write();
    void handle_close();
    void handle_error();

    void send_inLoop(const string& buf);
    //void shutdown();
    void shutdown_inLoop();

private:
    EventLoop *loop_; //所属subloop，非baseloop
    const string name_;

    atomic_int state_;
    bool reading_;

    unique_ptr<Socket> socket_;
    unique_ptr<Channel> channel_;

    const InetAddress localaddr_;
    const InetAddress peeraddr_;

    ConnectionCallback connection_callback_;        //有新连接时回调
    MessageCallback message_callback_;              //有读写消息的回调
    WriteCompleteCallback write_complete_callback_; //消息发送完以后的回调
    CloseCallback close_callback_;

    HighWaterMarkCallback highwater_callback_;
    size_t highwater_mark_; //高水位标志

    Buffer input_buffer_;  //接收数据的缓冲区
    Buffer output_buffer_; //发送数据的缓冲区
};