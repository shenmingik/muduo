#pragma once

#include <string>
#include <iostream>
using namespace std;

class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int64_t times);
    //获取当前时间
    static TimeStamp now();
    //转换为字符串
    string to_string();

private:
    int64_t times_;
};