#include <memory>
#include "EventLoopThread.hpp"
#include "EventLoop.hpp"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const string &name)
    : loop_(nullptr), exiting_(false), thread_(bind(&EventLoopThread::thread_function, this), name), thread_mutex_(), condition_(), callback_function_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::start_loop()
{
    thread_.start(); //启动线程

    EventLoop *loop = nullptr;
    {
        unique_lock<mutex> lock(thread_mutex_);
        while (loop_ == nullptr)
        {
            condition_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

//启动的线程中执行以下方法
void EventLoopThread::thread_function()
{
    EventLoop loop; //创建一个独立的EventLoop，和上面的线程是一一对应 one loop per thread

    if (callback_function_)
    {
        callback_function_(&loop);
    }

    {
        unique_lock<mutex> lock(thread_mutex_);
        loop_ = &loop;
        condition_.notify_one();
    }

    loop.loop(); //开启事件循环

    //结束事件循环
    unique_lock<mutex> lock(thread_mutex_);
    loop_ = nullptr;
}