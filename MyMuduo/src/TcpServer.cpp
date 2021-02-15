#include <strings.h>
#include "TcpServer.hpp"

using namespace placeholders;

#define BUFFER_SIZE64 64

EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenaddr, const string &name, Option option)
    : loop_(CheckLoopNotNull(loop)), ip_port_(listenaddr.get_ip_port()), name_(name), acceptor_(new Acceptor(loop, listenaddr, option == k_reuse_port)), thread_pool_(new EventLoopThreadPool(loop, name_)), connection_callback_(), message_callback_(), next_conn_id_(1), started_(0)
{
    //当有新连接会执行new_connection
    acceptor_->set_new_connection_callback(bind(&TcpServer::new_connection, this, _1, _2));
}
TcpServer::~TcpServer()
{
    for (auto &it : connections_)
    {
        TcpConnectionPtr conn(it.second); //出右括号，这个局部shared_ptr智能只针对新，出右括号，自动释放new出来的tcpconnection对象
        it.second.reset();

        conn->get_loop()->run_in_loop(bind(&TcpConnection::destory_connect, conn));
    }
}

//设置底层subloop的个数
void TcpServer::set_thread_num(int thread_num)
{
    thread_pool_->set_threadNum(thread_num);
}

//开启服务器监听
void TcpServer::start()
{
    if (started_++ == 0) //防止被多次启动
    {
        thread_pool_->start(thread_init_callback_);
        loop_->run_in_loop(bind(&Acceptor::listen, acceptor_.get()));
    }
}

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

void TcpServer::remove_connection(const TcpConnectionPtr &conn)
{
    loop_->run_in_loop(bind(&TcpServer::remove_connection_inLoop, this, conn));
}

void TcpServer::remove_connection_inLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("tcp server::remove connection in loop[%s]-connecion[%s]\n", name_.c_str(), conn->get_name().c_str());

    connections_.erase(conn->get_name());
    EventLoop *ioloop = conn->get_loop();
    ioloop->queue_in_loop(bind(&TcpConnection::destory_connect, conn));
}