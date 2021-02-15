#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

using namespace std;

//封装socket
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, string ip = "127.0.0.1");

    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    string get_ip() const;
    string get_ip_port() const;
    uint16_t get_port() const;

    void set_sockaddr(const sockaddr_in &addr) { addr_ = addr; }
    const sockaddr_in *get_sockaddr() const { return &addr_; }

private:
    sockaddr_in addr_;
};