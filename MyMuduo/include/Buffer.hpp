#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "NonCopyable.hpp"

using namespace std;

//网络库底层缓冲器类型
class Buffer : NonCopyable
{
public:
    explicit Buffer(size_t init_size = k_init_size)
        : buffer_(k_cheap_prepend + init_size), read_index_(k_cheap_prepend), write_index_(k_cheap_prepend)
    {
    }

    //返回可读的长度
    size_t readable_bytes() const { return write_index_ - read_index_; }

    //返回可写的长度
    size_t wirteable_bytes() const { return buffer_.size() - write_index_; }

    //返回头长度
    size_t prependable_bytes() const { return read_index_; }

    //返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + read_index_; }

    //缓冲区readindex 偏移
    void retrieve(size_t len)
    {
        //只读取一部分
        if (len < readable_bytes())
        {
            read_index_ += len;
        }
        else //全读
        {
            retrieve_all();
        }
    }

    //缓冲区复位
    void retrieve_all()
    {
        read_index_ = write_index_ = k_cheap_prepend;
    }

    //读取所有数据
    string retrieve_all_asString()
    {
        return retrieve_as_string(readable_bytes()); //可读取数据的总长度
    }

    //读取len长度数据
    string retrieve_as_string(size_t len)
    {
        string result(peek(), len);
        retrieve(len); //上面一句把缓冲区中可读的数据，已经读取，这里对缓冲区进行复位操作
        return result;
    }

    //保证缓冲区有这么长的可写空间
    void ensure_writeable_bytes(size_t len)
    {
        if (wirteable_bytes() < len)
        {
            makespace(len);
        }
    }

    //返回可写数据地址
    char *begin_write()
    {
        return begin() + write_index_;
    }

    //忘缓冲区中添加数据
    void append(const char *data, size_t len)
    {
        ensure_writeable_bytes(len);
        copy(data, data + len, begin_write());

        write_index_ += len;
    }

    //从fd中读取数据
    ssize_t readfd(int fd, int *save_errno);

    //通过fd发送数据
    ssize_t writefd(int fd, int *save_errno);

public:
    static const size_t k_cheap_prepend = 8;
    static const size_t k_init_size = 1024;

private:
    //返回缓冲区起始地址
    char *begin()
    {
        return &*buffer_.begin();
    }

    const char *begin() const
    {
        return &*buffer_.begin();
    }

    //扩容函数
    void makespace(size_t len)
    {
        if (wirteable_bytes() + prependable_bytes() < len + k_cheap_prepend)
        {
            buffer_.resize(write_index_ + len);
        }
        else
        {
            //把可读缓冲区数据前移，前部空余与可写部分连在一起
            size_t readable = readable_bytes();
            copy(begin() + read_index_, begin() + write_index_, begin() + k_cheap_prepend);

            read_index_ = k_cheap_prepend;
            write_index_ = read_index_ + readable;
        }
    }

private:
    vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
};
