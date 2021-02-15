#pragma once

#include <string>
#include "NonCopyable.hpp"

using namespace std;

#define BUFFER_SIZE 1024

//日志级别 INFO ERROR FATAL DEBUG
enum EnLogLevel
{
    INFO,  //普通打印信息
    ERROR, //错误信息
    FATAL, //core信息
    DEBUG, //调试信息
};

//LOG_INFO("%s,%d",arg1,arg2)

#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.set_log_level(EnLogLevel::INFO);           \
        char buf[BUFFER_SIZE] = {0};                      \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.set_log_level(EnLogLevel::ERROR);          \
        char buf[BUFFER_SIZE] = {0};                      \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.set_log_level(EnLogLevel::FATAL);          \
        char buf[BUFFER_SIZE] = {0};                      \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#if MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.set_log_level(EnLogLevel::DEBUG);          \
        char buf[BUFFER_SIZE] = {0};                      \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

//输出一个日志类 单例模式
class Logger : NonCopyable
{
public:
    //获取唯一实例对象
    static Logger &instance();

    //设置日志级别
    void set_log_level(EnLogLevel level);

    //写日志
    void log(string msg);

private:
    Logger() {}

private:
    int log_level_;
};