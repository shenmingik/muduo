# muduo
基于C++11的muduo网络库

作者：shenmingik
邮箱：2107810343@qq.com
时间：2021/1/26 22:17
开发环境：Ubuntu VS Code
编译器：g++
编程语言：C++
源码链接：
[微云链接](https://share.weiyun.com/TvmQb52d)
[GitHub链接](https://github.com/harker-k/muduo)
# 写在前面
## 项目编译问题
项目编译时基于cmake的，在源码链接中有CMakeLists.txt文件，下载完之后直接点击那个编译即可。

## 库安装的问题
在源码链接的`/lib`目录中有一个autobuild.sh文件，用`chmod +x autobuild.sh`命令给它加上执行权限之后，执行就可。

> 注：博主的是Ubuntu系统，其他系统可以进去把地址给改一下

## 项目测试代码
这里简单的写了一个回显服务器用于测试，在源码链接的`/example`目录下，大家可以下载自己测试一下。

## 关于压力测试
由于我这里基本就是重写了一下原来的muduo + 懒（不是主要原因）

所以

压力测试可以直接参考陈硕大神做的：

[muduo 与 libevent2 吞吐量对比](https://blog.csdn.net/Solstice/article/details/5864889)
[muduo 与 boost asio 吞吐量对比](https://blog.csdn.net/Solstice/article/details/5863411)

# 项目概述
这个项目呢，是将陈硕大神的muduo网络库源码中核心代码部分重新写了一遍，将原来依赖boost库的地方都替换成了C++ 11语法。算是博主对muduo网络库达成更好理解的一个产品吧。


## muduo网络库的reactor模型
在muduo网络库中，采用的是**reactor**模型，那么，什么是reactor模型呢？

> **Reactor：**
> 即非阻塞同步网络模型，可以理解为，向内核去注册一个感兴趣的事件，事件来了去通知你，你去处理
> **Proactor：**
> 即异步网络模型，可以理解为，向内核去注册一个感兴趣的事件及其处理handler，事件来了，内核去处理，完成之后告诉你

## muduo的设计
reactor模型在实际设计中大致是有以下几个部分：

- Event：事件
- Reactor： 反应堆
- Demultiplex：多路事件分发器
- EventHandler：事件处理器

在muduo中，其**调用关系**大致如下

-	将事件及其处理方法注册到reactor，reactor中存储了连接套接字connfd以及其感兴趣的事件event
-	reactor向其所对应的demultiplex去注册相应的connfd+事件，启动反应堆
-	当demultiplex检测到connfd上有事件发生，就会返回相应事件
-	reactor根据事件去调用eventhandler处理程序 
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210214113852361.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3NoZW5taW5neHVlSVQ=,size_16,color_FFFFFF,t_70)
而上述的，是在一个reactor反应堆中所执行的大致流程，其在muduo代码中**包含关系**如下（椭圆圈起来的是类）：

可以看到，EventLoop其实就是我们的reactor，其执行在一个Thread上，实现了one loop per thread的设计。
每个EventLoop中，我们可以看到有一个Poller和很多的Channel，Poller在上图调用关系中，其实就是demultiplex（多路事件分发器）,而Channel对应的就是event（事件）
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210214120009308.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3NoZW5taW5neHVlSVQ=,size_16,color_FFFFFF,t_70)
现在，我们大致明白了muduo每个reactor的设计，但是作为一个支持高并发的网络库，单线程 往往不是一个好的设计。

muduo采用了和Nginx相似的操作，有一个main reactor通过accept组件负责处理新的客户端连接，并将它们分派给各个sub reactor，每个sub reactor则是负责一个连接的读写等工作。
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210214111718808.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3NoZW5taW5neHVlSVQ=,size_16,color_FFFFFF,t_70)
# muduo各个类
明白了muduo的细节之后，我们对muduo的剖析就更为容易。

## 辅助类
这个类别的类与网络实现没有太大关系，只是用来辅助网络库的实现了

### NonCopyable
这个类将拷贝和赋值构造函数给delete掉，提供了一个**不可拷贝**的基类

```cpp
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
```
### TimeStamp
这个类用于给网络库提供系统时间，我这里用的是`time（nullptr）`函数

```cpp
    TimeStamp();
    explicit TimeStamp(int64_t times);
    //获取当前时间
    static TimeStamp now();
    //转换为字符串
    string to_string();
```
### Logger
这个是日志类,采用的是饿汉式的单例模式，用于打印网络库运行过程中的日志信息，主要分为四个级别

- INFO:	正常的日志输出
- ERROR:	有错误的日志输出，但是程序还可以运行
- FATAL:	有错误的日志输出，程序不可运行，直接exit
- DEBUG:	用于调试得到错误信息

同时也往外提供了四个宏函数用于打印信息：`LOG_INFO、 LOG_ERROR、LOG_FATAL、LOG_DEBUG`，由于四个函数相似度较大，我这里就放出一个函数LOG_INFO
```cpp
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.set_log_level(EnLogLevel::INFO);           \
        char buf[BUFFER_SIZE] = {0};                      \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
```
我们可以看到，这里是对输入数据进行了处理然后调的logger的log函数进行打印。

```cpp
    //获取唯一实例对象
    static Logger &instance();

    //设置日志级别
    void set_log_level(EnLogLevel level);

    //写日志
    void log(string msg);
```
### Buffer
这个是muduo网络库中底层的数据缓冲类型，模仿java中netty的设计，其有一个prepend、read、write三个标志，划分了缓冲区的数据。
其中perpend-read之间是一个头部的标志位，read-write是可读数据，write-末尾是可写数据。

应用将数据写入到网络库的Buffer缓冲区，然后Buffer缓冲区再写到TCP的缓冲区，最后再发送。
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210214125729501.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3NoZW5taW5neHVlSVQ=,size_16,color_FFFFFF,t_70)

```cpp
数据：
    vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
方法：
    //返回可读的长度
    size_t readable_bytes() ;
    //返回可写的长度
    size_t wirteable_bytes();
    //返回头长度
    size_t prependable_bytes();
    //返回缓冲区中可读数据的起始地址
    const char *peek();
    //缓冲区readindex 偏移
    void retrieve(size_t len);
    //缓冲区复位
    void retrieve_all();
    //读取所有数据
    string retrieve_all_asString();
    //读取len长度数据
    string retrieve_as_string(size_t len);
    //保证缓冲区有这么长的可写空间
    void ensure_writeable_bytes(size_t len);
    //返回可写数据地址
    char *begin_write();
    //忘缓冲区中添加数据
    void append(const char *data, size_t len);
    //从fd中读取数据
    ssize_t readfd(int fd, int *save_errno);
    //通过fd发送数据
    ssize_t writefd(int fd, int *save_errno);
private:
    //返回缓冲区起始地址
    char *begin();
    //扩容函数
    void makespace(size_t len);
```
## Reactor中类
这个类别中主要讲解reactor中要实现的类
### InetAddress
这个类封装了socket所要绑定的**ip地址**和**端口号**，比较简单

```cpp
    string get_ip() const;
    string get_ip_port() const;
    uint16_t get_port() const;

    void set_sockaddr(const sockaddr_in &addr) { addr_ = addr; }
    const sockaddr_in *get_sockaddr() const { return &addr_; }
```
### Channel
这个类中主要是封装了sockfd及其所感兴趣的事件，还有发生事件所要调用的回调函数。

```cpp
数据：
    EventLoop *loop_;	//所属loop
    const int fd_;
    int events_;      //fd感兴趣事件
    int real_events_; //poller 具体发生的事件
    int index_;

    weak_ptr<void> tie_; //观察当前channel的存在状态
    bool tied_;          //判断tie_是否绑定过

    //发生事件所要调用的具体事件的回调操作
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
 方法：
     //fd得到poller通知以后，根据具体发生的事件，调用相应的回调
     //其实调用的handle_event_withGuard
    void handle_event(TimeStamp receive_time);
    //防止channel被remove掉，channel还在执行回调
    //一个tcpconnection新连接创建的时候，调用tie
    void tie(const shared_ptr<void> &);
    //得到socket套接字
    int get_fd();
    //得到感兴趣事件
    int get_events();
    //设置真正发生的事件,poller监听到事件然后设置real_event
    int set_revent(int event);
    //判断该fd是否设置过感兴趣事件
    bool is_noneEvent();
    //返回所属eventloop
    EventLoop *owner_loop();
    //在channel所属的eventloop中删除自己
    void remove();
public:
    //设置fd感兴趣事件
    void enable_reading();
    void dis_enable_reading();
    void enable_writing();
    void dis_enable_writing();
    void dis_enable_all();
public:
    //返回fd当前感兴趣事件状态
    bool is_none_event();
    bool is_writting();
    bool is_reading();
public:
    //设置发生不同事件的回调操作
	......
private:
    //与poller更新fd所感兴趣事件
    void update();
    //根据发生的具体事件调用相应的回调操作
    void handle_event_withGuard(TimeStamp receive_time);
```
### EpollPoller
这个封装了epoll，也就是底层的demultiplex（多路事件分发），里面包含了一个指向Channel的指针，以及自己在内核事件表中的fd

```cpp
数据：
    int epollfd_;
    //key: fd  value:fd所属channel
    ChannelMap channels_;		//unordered_map<int, Channel *>;
方法：
	//调用epoll_wait，并将发生事件的channel填写到形参active_channel中
	TimeStamp poll(int timeout, ChannelList *active_channels) override;
	//往channel_map中添加channel
    void update_channel(Channel *channel);
    //channel_map中删除channel
    void remove_channel(Channel *channel);
private:
    //填写活跃的链接
    void fill_active_channels(int events_num, ChannelList *active_channels) const;
    //更新channel，调用epoll_ctl
    void update(int operation, Channel *channel);
```
### EventLoop
这个是事件循环类，主要包含两个组件 ----- Poller以及Channel。

```cpp
数据：
    atomic_bool looping_;
    atomic_bool quit_; //标志退出loop循环

    const pid_t threadId_;       //记录当前loop所在线程id
    TimeStamp poll_return_time_; //poller返回的发生事件的时间点
    unique_ptr<Poller> poller_;

    int wakeup_fd;                       //当main loop获取一个新用户的channel，通过轮询算法，选择一个subloopp，通过该成员唤醒subloop，处理channel
    unique_ptr<Channel> wakeup_channel_; //包装wakefd

    ChannelList active_channels; //eventloop 所管理的所有channel

    atomic_bool calling_pending_functors_; //标识当前loop是否有需要执行的回调操作
    vector<Functor> pending_Functors_;     //loop所执行的所有回调操作

    mutex functor_mutex_; //保护pending_functors
```
可以看到，其有一个指向Poller的指针，以及一个存储Channel的容器，ChannelList。

值得注意的是，这里还有一个wakeup_fd，这是干什么的呢？

**唤醒** eventloop用的！

试想一下，如果一个eventloop一直处于epoll_wait的阻塞状态，那么我main reactor怎么去给他分配新的连接？我不得叫醒它？

在libevent中也有相似的组件，不过用的是sockpair，相当于在本地创建socket进行通信，而muduo中用的则是更加高效的eventfd（事件驱动，更快，8字节缓冲区，更省）。上层模块通过往eventfd中去写，触发写事件，内核就自然而然的将相应的eventloop唤醒了。

```cpp
方法：
    //开启事件循环
    void loop();
    //退出事件循环
    void quit();
    TimeStamp get_poll_returnTime() const { return poll_return_time_; }
    //在当前loop中执行cb
    void run_in_loop(Functor cb);
    //把cb放入队列中,唤醒loop所在线程执行cb（pending_functor）
    void queue_in_loop(Functor cb);
    //唤醒loop所在线程
    void wakeup();
    //poller的方法
    void update_channel(Channel *channel);
    void remove_channel(Channel *channel);
    bool has_channel(Channel *channel);
    //判断eventloop对象是否在自己的线程中
    bool is_in_loopThread() const;
private:
    void handle_read();         //wake up
    void do_pending_functors(); //执行回调
```
现在我们大概对已经介绍的类有了一点眉目了，其实也就是对epoll的一个封装，原来的EpollLoop在epoll_create，注册各个channel之后，就处于epoll_wait处于阻塞状态。如果这个时候，之前的channel没有事件发生，而上层又想唤醒当前的EventLoop去执行新的连接，就调用wakeup，唤醒当前的EventLoop。

### Thread
这个类，在原来的muduo中使用linux系统调用pthread_create那样写的较为繁琐，这里直接使用了C++ 11的thread类。相对来说，也比较简单了。

```cpp
数据：
    bool started_;
    bool joined_;
    shared_ptr<thread> thread_;

    pid_t tid_;
    ThreadFunc function_;
    string name_;
方法：
    void start();
    void join();
    bool is_started() const { return started_; }
    pid_t get_tid() const { return tid_; }
    const string &get_name() const { return name_; }
    static int get_thread_nums() { return thread_nums_; }
private:
    void set_default_name();
```
### EventLoopThread
都说muduo网络库的核心是**one loop per thread**，即一个线程一个eventloop。其实现的秘密就蕴藏在EventLoopThread类中。

```cpp
数据：
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    mutex thread_mutex_;
    condition_variable condition_;
    ThreadInitCallback callback_function_;
方法：
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const string &name = string());
    ~EventLoopThread();
    EventLoop *start_loop();
private:
    void thread_function();
```
是不是很少，秘密藏在哪儿也就能直接看出来吧？

**start_loop 与 thread_function**！

```cpp
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
```
而thread_function是通过初始化列表的方式，绑定在线程所要执行的函数当中。

### EventLoopThreadPool
这个类，在之前的图中你可以理解为sub reactor池，通过设置thread_nums，可以创建相应数量的sub reactor。

```cpp
数据：
	EventLoop *baseloop_;		//main loop
    string name_;
    bool started_;
    int thread_nums_;
    int next_;
    vector<unique_ptr<EventLoopThread>> threads_; //所有线程，即所有subReactor
    vector<EventLoop *> loops_;                   //每个线程里面对应的loop事件循环
方法：
    //设定线程数量，一般应该和CPU核心数相同
    void set_threadNum(int thread_num) ;
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    //如果工作在多线程，baseloop默认轮询方式分配channel给subloop
    EventLoop *get_nextEventLoop();
    vector<EventLoop *> get_allLoops();
    bool get_started() const;
    string get_name() const;
```
## main Reactor
这个类别则主要是main reactor中实现的类，跟客户端的分发有关。

### Socket
这个类也是对socket编程的封装，跟channel不同的是，它封装的是socket编程流程，包括bind、listen、accept 以及设置 socket的属性信息等

```cpp
数据：
    const int sockfd_;
方法：
    int get_fd() { return sockfd_; }
    void bind_address(const InetAddress &loacladdr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdown_write();
    void set_tcp_noDelay(bool on);
    void set_reuseAddr(bool on);
    void set_reusePort(bool on);
    void set_keepAlive(bool on);
```
### Acceptor
这个组件在上图是特别点出来的，属于main reactor，用于分发客户端连接。

```cpp
数据：
    EventLoop *loop_; //acceptor用的用户定义的那个baseloop，也就是mainloop
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connetion_callback_;
    bool listenning_;
```
可以看到，它本身也是一个socket，属于listen_fd，用于监听客户端的连接事件。当新用户连接的时候，就会执行它的new_connection_callback。那么这个回调又干了什么呢？

```cpp
    void set_new_connection_callback(const NewConnectionCallback &cb)
    {
        new_connetion_callback_ = cb;
    }
```
阅读源码我们知道，这个是上层给他设置的，也就是TcpServer中。所以说，具体的敢了什么得等会儿才知道。

```cpp
方法：
	void set_new_connection_callback(const NewConnectionCallback &cb);
    bool is_listening();
    void listen();
 private:
    void handle_read();
```
这里可以着重强调一下handle_read这个函数，它是accept触发读事件所设定的回调函数。

```cpp
//listenfd 有事件发生，即新用户连接
void Acceptor::handle_read()
{
    InetAddress peeraddr;

    int connfd = accept_socket_.accept(&peeraddr);
    if (connfd > 0)
    {
        if (new_connetion_callback_)
        {
            new_connetion_callback_(connfd, peeraddr); //轮询找到subloop，唤醒，分发当前新客户端的channel
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s : %s :%d accept error : %d!\n", __FILE__, __FUNCTION__, __LINE__, errno);
        //进程没有可用的fd
        if (errno == EMFILE)
        {
            LOG_ERROR("%s : %s :%d sockfd reached limit error : %d!\n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
}
```
看到了吧，它会将客户端的连接套接字进行一个信息记录，然后将这些信息传递到new_connection_callbak，执行它。所以说，这里执行了接受连接的命令，但是连接的分发还是藏在这个回调函数之中！
### TcpServer
这个可以说是整个网络库的入口，为了用于更加方便使用，我将所有头文件都包含在这里了。

```cpp
数据：
    EventLoop *loop_;		//用户传进来的loop，也就是mainloop
    const string ip_port_;
    const string name_;
    unique_ptr<Acceptor> acceptor_;               //运行在mainloop，主要是监听新连接事件
    shared_ptr<EventLoopThreadPool> thread_pool_; //one loop per thread
    ConnectionCallback connection_callback_;        //有新连接时回调
    MessageCallback message_callback_;              //有读写消息的回调
    WriteCompleteCallback write_complete_callback_; //消息发送完以后的回调
    ThreadInitCallback thread_init_callback_; //loop 线程初始化的回调
    atomic_int started_;
    int next_conn_id_;
    ConnectionMap connections_; //保存所有连接	unordered_map<string, TcpConnectionPtr>;
```
可以看到，它大抵上是有这么几个组件，acceptor、eventloopthreadpool、一些回调、以及一个存储channel连接信息的connectionmap。刚刚好和我们一开始就提到的图对应起来了。

```cpp
    //设置回调
    void set_thread_init_callback(const ThreadInitCallback &callback);
    void set_connection_callback(const ConnectionCallback &callback);
    void set_message_callback(const MessageCallback &callback);
    void set_write_complete_callback(const WriteCompleteCallback &callback);
    //设置底层subloop的个数
    void set_thread_num(int thread_num);
    //开启服务器监听
    void start();
private:
    void new_connection(int sockfd, const InetAddress &peeraddr);
    void remove_connection(const TcpConnectionPtr &conn);
    void remove_connection_inLoop(const TcpConnectionPtr &conn);
```
可以看到，最重要的其实就是`start()`以及三个私有函数了。首先来看看start函数做了什么？

```cpp
//开启服务器监听
void TcpServer::start()
{
    if (started_++ == 0) //防止被多次启动
    {
        thread_pool_->start(thread_init_callback_);
        loop_->run_in_loop(bind(&Acceptor::listen, acceptor_.get()));
    }
}
```
看，它其实就是把EventLoopThreadPool给启动了，然后调用acceptor的listen方法，去监听连接而来的套接字。

那么刚刚我们说的new_connection_callbak在哪儿，咋没看见？

```cpp
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenaddr, const string &name, Option option)
    : loop_(CheckLoopNotNull(loop)), ip_port_(listenaddr.get_ip_port()), name_(name), acceptor_(new Acceptor(loop, listenaddr, option == k_reuse_port)), thread_pool_(new EventLoopThreadPool(loop, name_)), connection_callback_(), message_callback_(), next_conn_id_(1), started_(0)
{
    //当有新连接会执行new_connection
    acceptor_->set_new_connection_callback(bind(&TcpServer::new_connection, this, _1, _2));
}
```
其实啊，在构造函数中被绑定了TcpServer的new_connection方法，也就是说，acceptor监听到新用户连接的时候，其实是执行TcpServer的new_connection方法。
那么这个方法又做了什么呢？

```cpp
//有一个新的客户端的连接，acceptor会执行这个回调
void TcpServer::new_connection(int sockfd, const InetAddress &peeraddr)
{
    //轮询算法，选择一个subloop管理channel
    EventLoop *ioloop = thread_pool_->get_nextEventLoop();

    char buffer[BUFFER_SIZE64] = {0};
    snprintf(buffer, sizeof(buffer), "-%s#%d", ip_port_.c_str(), next_conn_id_);
    ++next_conn_id_;
    string conn_name = name_ + buffer;

    LOG_INFO("tcp server:: new connection[%s] - new connection[%s] from %s\n", name_.c_str(), conn_name.c_str(), peeraddr.get_ip_port().c_str());

    //通过sockfd，获取其绑定的端口号和ip信息
    sockaddr_in local;
    bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("new connection get localaddr error\n");
    }

    InetAddress localaddr(local);

    //根据连接成功的sockfd，创建tcpc连接对象
    TcpConnectionPtr conn(new TcpConnection(ioloop, conn_name, sockfd, localaddr, peeraddr));

    connections_[conn_name] = conn;

    //下面回调是用户设置给tcpserver-》tcpconn-》channel-》poller-》notify channel
    conn->set_connection_callback(connection_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_write_complete_callback(write_complete_callback_);

    //设置如何关闭连接的回调
    conn->set_close_callback(bind(&TcpServer::remove_connection, this, _1));
    ioloop->run_in_loop(bind(&TcpConnection::establish_connect, conn));
}
```
去掉那些打印信息，我们总结一下：

1. 轮询算法选择一个sub reactor
2. 根据连接成功的sockfd，创建一个连接对象并加入到TcpServer的存储连接信息的map中
3. 给这个连接设置回调
4. 然后在main loop中执行estabilsh_connect

那么这个setabilsh_connect又干了什么呢？

这个我们就需要看TcpConnection这个类。稍后再说。

刚刚只说了连接，那么断开连接又做了什么呢？

还是在刚刚的代码中，设置了关闭连接的回调函数：

```cpp
    //设置如何关闭连接的回调
    conn->set_close_callback(bind(&TcpServer::remove_connection, this, _1));
```
可以看到，调用的其实是TcpServer的remove_connection

```cpp
void TcpServer::remove_connection(const TcpConnectionPtr &conn)
{
    loop_->run_in_loop(bind(&TcpServer::remove_connection_inLoop, this, conn));
}
```
可以看到，客户端的连接和关闭都是在main loop中执行的，只有读写事件是在sub loop中执行。而关闭连接的回调绕了一下，最终调用的是remove_connection_inloop

```cpp
void TcpServer::remove_connection_inLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("tcp server::remove connection in loop[%s]-connecion[%s]\n", name_.c_str(), conn->get_name().c_str());

    connections_.erase(conn->get_name());
    EventLoop *ioloop = conn->get_loop();
    ioloop->queue_in_loop(bind(&TcpConnection::destory_connect, conn));
}
```
它就做了两件事：

1. 删除连接map中的信息
2. 执行TcpConnection中的destory_connect函数

好了，还是TcpConnection这个类。

### TcpConnection
```cpp
数据：
   	EventLoop *loop_; //所属subloop，非baseloop
    const string name_;
    atomic_int state_;
    bool reading_;
    unique_ptr<Socket> socket_;
    unique_ptr<Channel> channel_;
    const InetAddress localaddr_;
    const InetAddress peeraddr_;
    ConnectionCallback connection_callback_;        //有新连接时回调
    MessageCallback message_callback_;              //有读写消息的回调
    WriteCompleteCallback write_complete_callback_; //消息发送完以后的回调
    CloseCallback close_callback_;
    HighWaterMarkCallback highwater_callback_;
    size_t highwater_mark_; //高水位标志
    Buffer input_buffer_;  //接收数据的缓冲区
    Buffer output_buffer_; //发送数据的缓冲区
```

 可以看到，其实它也是一个包装，将底层的包装，然后给用户去使用。
 

```cpp
方法：
    EventLoop *get_loop() const { return loop_; }
    const string &get_name() const { return name_; }
    const InetAddress &get_localaddr() const { return localaddr_; }
    const InetAddress &get_peeraddr() const { return peeraddr_; }
    bool connected() { return state_ == k_connected; }
    void set_state(StateE state) { state_ = state; }
    
    //发送数据
    void send(const string& buf);
    //断开连接
    void shutdown();
    //建立连接
    void establish_connect();
    //销毁连接
    void destory_connect();

    //设置回调
	.......
private:
    void handle_read(TimeStamp receive_time);
    void handle_write();
    void handle_close();
    void handle_error();

    void send_inLoop(const string& buf);
    //void shutdown();
    void shutdown_inLoop();
```
其实重要的就是shutdown，establish_connect、destory_connect三个函数

```cpp
//断开连接
void TcpConnection::shutdown()
{
    if (state_ == k_connected)
    {
        set_state(k_disconnecting);
        loop_->run_in_loop(bind(&TcpConnection::shutdown_inLoop, this));
    }
}
void TcpConnection::shutdown_inLoop()
{
    //当前outputbuffer 中数据 全部发送完
    if (!channel_->is_writting())
    {
        socket_->shutdown_write(); //关闭写端      后续调用handle close
    }
}
```
也是做了两件事：
1. 设置此连接为关闭状态
2. 关闭底层套接字的写端

```cpp
//建立连接
void TcpConnection::establish_connect()
{
    set_state(k_connected);
    channel_->tie(shared_from_this());
    channel_->enable_reading(); //向poller注册channel的epollin事件

    //新连接建立
    connection_callback_(shared_from_this());
}
```
这个则是在底层把这个信息设置成对读事件感兴趣，然后调用用户给它传入的回调函数。

```cpp
/销毁连接
void TcpConnection::destory_connect()
{
    if (state_ == k_connected)
    {
        set_state(k_disconnected);
        channel_->dis_enable_all(); //把channel所有感兴趣的事件，从poller中delete
    }
    channel_->remove(); //从poller中删除掉
}
```
这个也是，向底层的Polller删除掉

# 总结
现在，我们应该对整个muduo有了大致的掌握

1. 用户创建一个main loop，主线程作为main reactor
2. 给TcpServer设置连接和读写事件回调，TcpServer再给TcpConnection设置回调，这个回调是发生事件用户设置要执行的，TcpConnection再给channel设置回调，在发生事件时，会先执行这个回调，再执行用户设置的回调
3. TcpServer根据用户设置传入的线程数，去pool中开启几个线程，如果没有设置，那么用户传入的main loop还要承担读写事件的任务。
4. 当有新连接进来时，创建一个实例对象，然后由Acceptor去轮询唤醒一个sub reactor给它服务
5. 同时，每个sub reactor在服务时，其所包含的那个Poller如果没有事件就会处于循环阻塞状态，发生事件之后，根据类型再去执行响应的回调操作

# 参考文献
	[1] 陈硕.发布一个基于 Reactor 模式的 C++ 网络库.CSDN.2010.08
