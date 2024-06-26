/*
 * @Author: cijliu
 * @Date: 2021-02-11 14:17:59
 * @LastEditTime: 2021-02-23 18:01:54
 */
#include "net.h"
#include <iostream>

#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"


TcpServer::TcpServer() {}

TcpServer::~TcpServer() {}

int TcpServer::Init(tcp_t *tcp, int port)
{
    memset(tcp, 0, sizeof(tcp_t));
    tcp->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp->sock == -1) {
        printf("create socket failed : %d\n", errno);
        return -1;
    }
    memset(&tcp->addr, 0, sizeof(struct sockaddr_in));
    tcp->addr.sin_family      = AF_INET;
    tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp->addr.sin_port        = htons(port);

    int opt = 1;
    setsockopt(tcp->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(tcp->sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    int ret = bind(tcp->sock, (struct sockaddr *)&tcp->addr, sizeof(struct sockaddr_in));
    if (ret) {
        printf("bind socket to address failed : %d\n", errno);
        close(tcp->sock);
        return -1;
    }
    ret = listen(tcp->sock, TCP_MAX_CLIENT); // XXX
    if (ret) {
        printf("listen socket failed : %d\n", errno);
        close(tcp->sock);
        return -1;
    }
    tcp->port = port;
    return 0;
}

int TcpServer::WaitClient(tcp_t *tcp)
{
    int i             = 0;
    socklen_t addrlen = sizeof(tcp->addr);
    while (tcp->client[i] != 0 && i < TCP_MAX_CLIENT) {
        i++;
    }
    tcp->client[i] = accept(tcp->sock, (struct sockaddr *)&tcp->addr, &addrlen);
    return tcp->client[i];
}

int TcpServer::CloseClient(tcp_t *tcp, int client)
{
    int i;
    while (tcp->client[i] != client && i < TCP_MAX_CLIENT) {
        i++;
    }
    close(tcp->client[i]);
    tcp->client[i] = 0;
    return 0;
}

int TcpServer::SendMsg(tcp_t *tcp, int client, char *data, int len)
{
    // spdlog::info("send to client fd = {} msg {} len {}", client, data, len);
    int i = 0;
    while (tcp->client[i] != client && i < TCP_MAX_CLIENT) {
        i++;
    }

    if (i >= TCP_MAX_CLIENT) {
        spdlog::error("illegal client fd = {}", client);
        return -1;
    }

    if (client <= 0) {
        spdlog::error("client fd = {}", client);
        return -1;
    }

    if (len <= 0) {
        spdlog::error("data len = {}", len);
        return -1;
    }

    int ret = send(client, data, len, 0);
    if (ret < 0) {
        spdlog::error("tcp send fail ret = {}", ret);
    }
    return ret;
}

int TcpServer::ReceiveMsg(tcp_t *tcp, int client, uint8_t *data, int len)
{
    int i = 0;
    while (tcp->client[i] != client && i < TCP_MAX_CLIENT)
        i++;
    if (i >= TCP_MAX_CLIENT) {
        return -1;
    }
    return read(client, data, len);
}

int TcpServer::Deinit(tcp_t *tcp)
{
    int i;
    for (i = 0; i < TCP_MAX_CLIENT; i++) {
        if (tcp->client[i] != 0) {
            close(tcp->client[i]);
            tcp->client[i] = 0;
        }
    }
    close(tcp->sock);
    return 0;
}
