#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include "Buffer.hpp"

//从fd中读取数据到buffer,返回读取字节数
ssize_t Buffer::readfd(int fd, int *save_errno)
{
    char extra_buffer[65536] = {0}; //栈上内存
    struct iovec vec[2];
    const size_t writeable = wirteable_bytes(); //buffer缓冲区可写数据大小

    vec[0].iov_base = begin() + write_index_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extra_buffer;
    vec[1].iov_len = sizeof(extra_buffer);

    const int iovcnt = writeable < sizeof(extra_buffer) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *save_errno = errno;
    }
    else if (n <= writeable)
    {
        write_index_ += n;
    }
    else //extra_buffer中也有数据
    {
        write_index_ = buffer_.size();
        //缓冲区写入数据并扩容
        append(extra_buffer, n - writeable);
    }
    return n;
}

//通过fd发送数据
ssize_t Buffer::writefd(int fd, int *save_errno)
{
    ssize_t n = write(fd, peek(), readable_bytes());
    if (n < 0)
    {
        *save_errno = errno;
    }
    return n;
}