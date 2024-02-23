/*
 * @Author: cijliu
 * @Date: 2021-02-11 21:20:26
 * @LastEditTime: 2021-02-26 09:29:41
 */

#include "net.h"

#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"


UdpServer::UdpServer() {}
UdpServer::~UdpServer() {}

int UdpServer::Init(udp_t *udp, int port)
{
    // int size = 50 * 1024;
    memset(udp, 0, sizeof(udp_t));
    udp->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp->sock == -1) {
        printf("create socket failed : %d\n", errno);
        return -1;
    }
    // setsockopt(udp->sock, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));

    int opt = 1;
    setsockopt(udp->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(udp->sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    memset(&udp->addr, 0, sizeof(struct sockaddr_in));
    udp->addr.sin_family      = AF_INET;
    udp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp->addr.sin_port        = htons(port);

    int ret = bind(udp->sock, (struct sockaddr *)&udp->addr, sizeof(struct sockaddr_in));
    if (ret) {
        printf("bind socket to address failed : %d\n", errno);
        close(udp->sock);
        return -1;
    }
    ret = fcntl(udp->sock, F_GETFL, 0);
    if (ret < 0) {
        printf("fcntl F_GETFL failed: %d\n", errno);
    } else {
        ret |= O_NONBLOCK;
        ret = fcntl(udp->sock, F_SETFL, ret);
        if (ret < 0) {
            printf("fcntl F_SETFL failed: %d\n", errno);
        }
    }
    udp->port = port;
    return 0;
}

int UdpServer::SendMsg(udp_t *udp, const char *ip, const int port, uint8_t *data, int len)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port        = htons(port);
    int ret;
    int retry = 3;
    do {
        ret = sendto(udp->sock, (const uint8_t *)data, len, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (ret != len) {
            // spdlog::error("udp send fail ret = {}", ret);
            // return ret;
        } else {
            return 0;
        }
        retry--;
    } while (ret != len && retry > 0);

    return -1;
}

int UdpServer::ReciveMsg(udp_t *udp, ip_t *ip, uint8_t *data, int len)
{
    struct sockaddr_in addr;
    socklen_t size = 0;
    ssize_t ret    = recvfrom(udp->sock, data, len, 0, (struct sockaddr *)&addr, &size);
    if (ret) {
        return -1;
    }
    snprintf(ip->ip, sizeof(ip->ip), "%s", inet_ntoa(addr.sin_addr));
    ip->port = ntohs(addr.sin_port);
    return size;
}

int UdpServer::Deinit(udp_t *udp)
{
    close(udp->sock);
    return 0;
}
