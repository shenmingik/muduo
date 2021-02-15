#pragma once

#include <memory>
#include <functional>
#include "TimeStamp.hpp"

class Buffer;
class TcpConnection;

using namespace std;

using TcpConnectionPtr = shared_ptr<TcpConnection>;
using ConnectionCallback = function<void(const TcpConnectionPtr &)>;
using CloseCallback = function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = function<void(const TcpConnectionPtr &)>;

using MessageCallback = function<void(const TcpConnectionPtr &, Buffer *, TimeStamp)>;

using HighWaterMarkCallback = function<void(const TcpConnectionPtr &, size_t)>;
