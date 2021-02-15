#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "NonCopyable.hpp"

class EventLoop;
class EventLoopThread;

using namespace std;

using ThreadInitCallback = function<void(EventLoop *)>;

class EventLoopThreadPool : NonCopyable
{
public:
    EventLoopThreadPool(EventLoop *baseloop, const string &name);
    ~EventLoopThreadPool();

    //设定线程数量，一般应该和CPU核心数相同
    void set_threadNum(int thread_num) { thread_nums_ = thread_num; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //如果工作在多线程，baseloop默认轮询方式分配channel给subloop
    EventLoop *get_nextEventLoop();

    vector<EventLoop *> get_allLoops();

    bool get_started() const { return started_; }
    string get_name() const { return name_; }

private:
    EventLoop *baseloop_;
    string name_;

    bool started_;
    int thread_nums_;
    int next_;
    vector<unique_ptr<EventLoopThread>> threads_; //所有线程，即所有subReactor
    vector<EventLoop *> loops_;                   //每个线程里面对应的loop事件循环
};