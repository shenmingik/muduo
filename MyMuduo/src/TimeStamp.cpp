#include <ctime>
#include "TimeStamp.hpp"

#define BUFFER_SIZE 128

TimeStamp::TimeStamp() : times_(0) {}

TimeStamp::TimeStamp(int64_t times) : times_(times) {}

//获取当前时间
TimeStamp TimeStamp::now()
{
    return TimeStamp(time(nullptr));
}

//转换为字符串
string TimeStamp::to_string()
{
    char buffer[BUFFER_SIZE] = {0};
    tm *times = localtime(&times_);
    //年-月-日 时：分：秒
    snprintf(buffer, BUFFER_SIZE, "%4d-%02d-%02d %02d:%02d:%02d",
             times->tm_year + 1900,
             times->tm_mon + 1,
             times->tm_mday,
             times->tm_hour,
             times->tm_min,
             times->tm_sec);

    return buffer;
}

/*
#include<iostream>
int main()
{
    cout<<TimeStamp::now().to_string()<<endl;
}
*/
