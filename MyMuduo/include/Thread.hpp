#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <atomic>
#include "NonCopyable.hpp"

using namespace std;

using ThreadFunc = function<void()>;

class Thread : NonCopyable
{
public:
    explicit Thread(ThreadFunc, const string &name = string());
    ~Thread();

    void start();
    void join();

    bool is_started() const { return started_; }
    pid_t get_tid() const { return tid_; }
    const string &get_name() const { return name_; }

    static int get_thread_nums() { return thread_nums_; }

private:
    void set_default_name();

    bool started_;
    bool joined_;
    shared_ptr<thread> thread_;

    pid_t tid_;
    ThreadFunc function_;
    string name_;

    static atomic_int thread_nums_;
};