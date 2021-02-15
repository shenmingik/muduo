#pragma once

#include <vector>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include "NonCopyable.hpp"
#include "TimeStamp.hpp"
#include "CurrentThread.hpp"
#include "Poller.hpp"
#include "Channel.hpp"

//class Poller;

using namespace std;

using Functor = function<void()>;
using ChannelList = vector<Channel *>;

/*
* 事件循环类，主要包含两大模块 channel poller（epoll的抽象）
*/
class EventLoop : NonCopyable
{
public:
    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    TimeStamp get_poll_returnTime() const { return poll_return_time_; }

    //在当前loop中执行cb
    void run_in_loop(Functor cb);
    //把cb放入队列中,唤醒loop所在线程执行cb（pending_functor）
    void queue_in_loop(Functor cb);

    //唤醒loop所在线程
    void wakeup();

    //poller的方法
    void update_channel(Channel *channel);
    void remove_channel(Channel *channel);
    bool has_channel(Channel *channel);

    //判断eventloop对象是否在自己的线程中
    bool is_in_loopThread() const { return threadId_ == Current_thread::tid(); }

private:
    void handle_read();         //wake up
    void do_pending_functors(); //执行回调

private:
    atomic_bool looping_;
    atomic_bool quit_; //标志退出loop循环

    const pid_t threadId_;       //记录当前loop所在线程id
    TimeStamp poll_return_time_; //poller返回的发生事件的时间点
    unique_ptr<Poller> poller_;

    int wakeup_fd;                       //当main loop获取一个新用户的channel，通过轮询算法，选择一个subloopp，通过该成员唤醒subloop，处理channel
    unique_ptr<Channel> wakeup_channel_; //包装wakefd

    ChannelList active_channels; //eventloop 所管理的所有channel

    atomic_bool calling_pending_functors_; //标识当前loop是否有需要执行的回调操作
    vector<Functor> pending_Functors_;     //loop所执行的所有回调操作

    mutex functor_mutex_; //保护pending_functors
};