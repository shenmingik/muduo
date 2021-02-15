#include <mymuduo/TcpServer.hpp>
#include <string>
#include <functional>

using namespace std;
using namespace placeholders;

class EchoServer
{
public:
    EchoServer(EventLoop *loop, InetAddress &addr, string name)
        : server_(loop, addr, name), loop_(loop)
    {
        //注册回调函数
        server_.set_connection_callback(bind(&EchoServer::on_connection, this, _1));
        server_.set_message_callback(bind(&EchoServer::on_message, this, _1, _2, _3));

        //设置线程数量
        server_.set_thread_num(3);
    }
    void start()
    {
        server_.start();
    }

private:
    //连接建立或者断开的回调
    void on_connection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("conn up: %s", conn->get_peeraddr().get_ip_port().c_str());
        }
        else
        {
            LOG_INFO("conn down: %s", conn->get_peeraddr().get_ip_port().c_str());
        }
    }

    //可读事件回调
    void on_message(const TcpConnectionPtr &conn, Buffer *buffer, TimeStamp time)
    {
        string msg = buffer->retrieve_all_asString();
        conn->send(msg);
        conn->shutdown();
    }

private:
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "echo 01");
    server.start();
    loop.loop(); //启动main loop的底层poller
    return 0;
}