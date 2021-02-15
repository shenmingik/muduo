#pragma once

#include <functional>
#include <memory>
#include "NonCopyable.hpp"
#include "TimeStamp.hpp"

class EventLoop;

using namespace std;

using EventCallback = function<void()>;
using ReadEventCallback = function<void(TimeStamp)>;

/*
*   相当于sockfd及其感兴趣event的集合
*   绑定了poller返回的事件
*/
class Channel : NonCopyable
{
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    //fd得到poller通知以后，根据具体发生的事件，调用相应的回调
    void handle_event(TimeStamp receive_time);

    //防止channel被remove掉，channel还在执行回调
    //一个tcpconnection新连接创建的时候，调用tie
    void tie(const shared_ptr<void> &);

    //得到socket套接字
    int get_fd() const { return fd_; }

    //得到感兴趣事件
    int get_events() const { return events_; }

    //设置真正发生的事件,poller监听到事件然后设置real_event
    int set_revent(int event) { real_events_ = events_; }

    //判断该fd是否设置过感兴趣事件
    bool is_noneEvent() const { return events_ == k_none_event_; }

    //返回所属eventloop
    EventLoop *owner_loop() { return loop_; }

    //在channel所属的eventloop中删除自己
    void remove();

    int index() { return index_; }
    void set_index(int index) { index_ = index; }

public:
    //设置fd感兴趣事件
    void enable_reading()
    {
        events_ |= k_read_event_;
        update();
    }
    void dis_enable_reading()
    {
        events_ &= ~k_read_event_;
        update();
    }

    void enable_writing()
    {
        events_ |= k_write_event_;
        update();
    }
    void dis_enable_writing()
    {
        events_ &= ~k_write_event_;
        update();
    }

    void dis_enable_all()
    {
        events_ = k_none_event_;
        update();
    }

public:
    //返回fd当前感兴趣事件状态
    bool is_none_event() const { return events_ == k_none_event_; }
    bool is_writting() const { return events_ & k_write_event_; }
    bool is_reading() const { return events_ & k_read_event_; }

public:
    //设置发生不同事件的回调操作
    void set_readcallback(ReadEventCallback callback) { read_callback_ = move(callback); }
    void set_writecallback(EventCallback callback) { write_callback_ = move(callback); }
    void set_closecallback(EventCallback callback) { close_callback_ = move(callback); }
    void set_errorcallback(EventCallback callback) { error_callback_ = move(callback); }

private:
    //与poller更新fd所感兴趣事件
    void update();

    //根据发生的具体事件调用相应的回调操作
    void handle_event_withGuard(TimeStamp receive_time);

private:
    static const int k_none_event_;
    static const int k_read_event_;
    static const int k_write_event_;

private:
    EventLoop *loop_;
    const int fd_;
    int events_;      //fd感兴趣事件
    int real_events_; //poller 具体发生的事件
    int index_;

    weak_ptr<void> tie_; //观察当前channel的存在状态
    bool tied_;          //判断tie_是否绑定过

    //发生事件所要调用的具体事件的回调操作
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};