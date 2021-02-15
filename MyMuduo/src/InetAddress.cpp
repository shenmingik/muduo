#include <strings.h>
#include <cstring>
#include "InetAddress.hpp"

#define BUFFER_SIZE 64

InetAddress::InetAddress(uint16_t port, string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

string InetAddress::get_ip() const
{
    char buffer[BUFFER_SIZE] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buffer, BUFFER_SIZE);
    return buffer;
}
string InetAddress::get_ip_port() const
{
    uint16_t port = get_port();

    char buffer[BUFFER_SIZE] = {0};
    sprintf(buffer, " : %u", port);

    return get_ip() + buffer;
}
uint16_t InetAddress::get_port() const
{
    return ntohs(addr_.sin_port);
}
