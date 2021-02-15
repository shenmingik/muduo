#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <unordered_map>
#include "EventLoop.hpp"
#include "EventLoopThreadPool.hpp"
#include "TcpConnection.hpp"
#include "Acceptor.hpp"
#include "InetAddress.hpp"
#include "NonCopyable.hpp"
#include "Callbacks.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"

using namespace std;

using ThreadInitCallback = function<void(EventLoop *)>;
using ConnectionMap = unordered_map<string, TcpConnectionPtr>;

class TcpServer : NonCopyable
{
public:
    enum Option
    {
        k_noReuse_port, //不可重用
        k_reuse_port,   //可重用
    };

public:
    TcpServer(EventLoop *loop, const InetAddress &listenaddr, const string &name, Option option = k_noReuse_port);
    ~TcpServer();

    //设置回调
    void set_thread_init_callback(const ThreadInitCallback &callback) { thread_init_callback_ = callback; }
    void set_connection_callback(const ConnectionCallback &callback) { connection_callback_ = callback; }
    void set_message_callback(const MessageCallback &callback) { message_callback_ = callback; }
    void set_write_complete_callback(const WriteCompleteCallback &callback) { write_complete_callback_ = callback; }

    //设置底层subloop的个数
    void set_thread_num(int thread_num);

    //开启服务器监听
    void start();

private:
    void new_connection(int sockfd, const InetAddress &peeraddr);

    void remove_connection(const TcpConnectionPtr &conn);

    void remove_connection_inLoop(const TcpConnectionPtr &conn);

private:
    EventLoop *loop_;
    const string ip_port_;
    const string name_;

    unique_ptr<Acceptor> acceptor_;               //运行在mainloop，主要是监听新连接事件
    shared_ptr<EventLoopThreadPool> thread_pool_; //one loop per thread

    ConnectionCallback connection_callback_;        //有新连接时回调
    MessageCallback message_callback_;              //有读写消息的回调
    WriteCompleteCallback write_complete_callback_; //消息发送完以后的回调

    ThreadInitCallback thread_init_callback_; //loop 线程初始化的回调

    atomic_int started_;
    int next_conn_id_;
    ConnectionMap connections_; //保存所有连接
};