#include <iostream>
#include "Logger.hpp"
#include "TimeStamp.hpp"

//获取唯一实例对象
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

//设置日志级别
void Logger::set_log_level(EnLogLevel level)
{
    log_level_ = level;
}

//写日志    [级别] time: msg
void Logger::log(string msg)
{
    switch (log_level_)
    {
    case INFO:
    {
        cout << "[INFO]";
        break;
    }
    case ERROR:
    {
        cout << "[ERROR]";
        break;
    }
    case FATAL:
    {
        cout << "[FATAL]";
        break;
    }
    case DEBUG:
    {
        cout << "[DEBUG]";
        break;
    }
    default:
        break;
    }

    //打印时间和msg
    cout << TimeStamp::now().to_string()
         << " : " << msg << endl;
}