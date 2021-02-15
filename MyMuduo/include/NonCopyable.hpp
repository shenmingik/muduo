#pragma once

/*
* 集成自noncopyable的的对象不可拷贝和赋值，但是可以构造析构
*/
class NonCopyable
{
public:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};