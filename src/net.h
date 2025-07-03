/*
 * @Author: cijliu
 * @Date: 2021-02-11 17:01:24
 * @LastEditTime: 2021-02-23 17:50:10
 */
#ifndef __NET_H__
#define __NET_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define TCP_MAX_CLIENT 128

typedef struct {
    char ip[16];
    int port;
} ip_t;

typedef struct {
    int sock;
    int port;
    struct sockaddr_in addr;
    int client[TCP_MAX_CLIENT];
} tcp_t;

typedef struct {
    int sock;
    int port;
    struct sockaddr_in addr;
} udp_t;

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    int Init(tcp_t *tcp, int port);
    int WaitClient(tcp_t *tcp);
    int CloseClient(tcp_t *tcp, int client);
    int SendMsg(tcp_t *tcp, int client, char *data, int len);
    int ReceiveMsg(tcp_t *tcp, int client, uint8_t *data, int len);
    int Deinit(tcp_t *tcp);
private:
};

class UdpServer
{
public:
    UdpServer();
    ~UdpServer();
    int Init(udp_t *udp, int port);
    int SendMsg(udp_t *udp, const char *ip, const int port, uint8_t *data, int len);
    int ReciveMsg(udp_t *udp, ip_t *ip, uint8_t *data, int len);
    int Deinit(udp_t *udp);
private:
};

#endif
